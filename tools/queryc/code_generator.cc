#include "imlab/infra/settings.h"
#include "code_generator.h"
#include <cstdlib>
#include <dlfcn.h>
#include <fstream>
#include <sstream>

namespace imlab {

namespace queryc {


    namespace {
        /// Global unique query id.
        size_t QUERY_ID = 0;
    } // namespace

    CodeGenerator::CodeGenerator(std::string include_dir_): include_dir(std::move(include_dir_))
    {
    }

    CodeGenerator::Function CodeGenerator::compile(OperatorPtr op)
    {
        if (!op) {
            throw std::runtime_error("Please pass an actual operator to the code generator.");
        }

        // Create handle for this query.
        size_t id = QUERY_ID++;
        std::string fname = "/tmp/imlabdb_query_" + std::to_string(id);
        std::string fname_h = fname + ".h";
        std::string fname_cc = fname + ".cc";
        std::string fname_so = fname + ".so";

        // PHASE 1: Generate the actual C++ code.
        generateCode(*op, id, fname_h, fname_cc);

        // Phase 2: Invoke the C++ compiler.
        invokeCompiler(fname_h, fname_cc, fname_so);

        // Phase 3: Invoke the dynamic linker and return the function handle.
        return invokeDLinker(fname_so);

    }

    void CodeGenerator::generateCode(Operator &op, size_t query_id, const std::string &fname_h, const std::string &fname_cc)
    {

        std::ofstream out_h(fname_h, std::ofstream::trunc);
        std::ofstream out_cc(fname_cc, std::ofstream::trunc);

        // Attach out_cc to codegen helper of operator.
        op.getHelper().AttachOutputStream(out_cc);

        // Write simple header.
        out_h << R"(
#pragma once

#include "gen/tpcc.h"
#include "imlab/database.h"
#include "imlab/infra/hash_table.h"
#include "imlab/storage/relation.h"
#include "imlab/infra/types.h"
#include <tuple>
#include <iostream>
#include <mutex>
#include <sstream>

extern "C" void executeQuery(void *);

)";

        // Prepare cc file.
        out_cc << "#include \"imlabdb_query_" << query_id << ".h\"\n";
        out_cc << R"(

using namespace imlab;

)";

        out_cc << "\nvoid executeQuery(void* db_ptr) \n";

        auto function_scope = op.getHelper().CreateScopedBlock();

        op.getHelper().Stmt() << "Database& db = *reinterpret_cast<Database*>(db_ptr);";

        // Generate the actual code.
        op.Prepare(std::set<const imlab::IU*>{}, nullptr);
        op.Produce();

    }

    void CodeGenerator::invokeCompiler(const std::string& fname_h, const std::string& fname_cc, const std::string& fname_so)
    {
        std::stringstream command;
        command << "clang++-11 " << fname_cc;
        // Initial attempt with rdynamic flags.
        // command << " -S -O3 -fPIC -Wl,-export_dynamic -std=c++17";
        command << " -O3 -g -fPIC -std=c++17";
        if constexpr (imlab::Settings::USE_TBB) {
            command << " -ltbb";
        }
        command << " -shared --include-directory /tmp --include-directory " << include_dir;
        command << " -o " << fname_so;
        std::system(command.str().c_str());
    }

    CodeGenerator::Function CodeGenerator::invokeDLinker(const std::string &fname_so)
    {
        // Call the dynamic linker.
        void* handle = dlopen(fname_so.c_str(), RTLD_NOW);
        if (!handle) {
            throw std::runtime_error("Error during dlopen: " + std::string(dlerror()));
        }

        auto fn=reinterpret_cast<void (*)(void *)>(dlsym(handle, "executeQuery"));
        if (!fn) {
            throw std::runtime_error("Error during dynamic symbol lookup: " + std::string(dlerror()));
        }

        return fn;
    }


} // namespace queryc

} // namespace imlab

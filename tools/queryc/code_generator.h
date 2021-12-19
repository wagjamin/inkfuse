#ifndef IMLAB_CODE_GENERATOR_H
#define IMLAB_CODE_GENERATOR_H

#include "imlab/algebra/operator.h"
#include <functional>

namespace imlab {

namespace queryc {

/// Redshift style global compilation cache for queries.
// TODO
struct CompilationCache {
private:
    struct Entry {
        /// Operator being cached. Needed for deep equality checks.
        OperatorPtr op;
        /// Handle to the compiled function.
        void* handle;
    };
};

/// Helper struct for code generation of an algebra tree.
struct CodeGenerator {
public:
    using Function = std::function<void(void*)>;

    /// Setup a central code generator. Receives the include directory where the imlab header files can be found.
    CodeGenerator(std::string include_dir_);

    /// Compile code for a given operator.
    /// Note: the generated code is put into /tmp/imlabdb_q<id>.[hpp|cpp]
    Function compile(OperatorPtr op);

private:
    /// Generate code for a given query.
    void generateCode(Operator& op, size_t query_id, const std::string& fname_h, const std::string& fname_cc);

    /// Invoke C++ compiler. Using clang++-11 atm.
    void invokeCompiler(const std::string& fname_h, const std::string& fname_cc, const std::string& fname_so);

    /// Invoke the dynamic linker.
    Function invokeDLinker(const std::string& fname_so);

    /// Compilation cache used to speed up query processing.
    CompilationCache cache;
    /// Backing include directory for imlab code.
    std::string include_dir;
};

} // namespace queryc

} // namespace imlab

#endif //IMLAB_CODE_GENERATOR_H

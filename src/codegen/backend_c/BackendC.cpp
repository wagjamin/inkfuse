#include "codegen/backend_c/BackendC.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <dlfcn.h>

namespace inkfuse {

void BackendProgramC::compileToMachinecode() {
   if (!was_compiled) {
      // Dump.
      dump();
      // Invoke the compiler.
      std::stringstream command;
      command << "clang-11 "
              << "/tmp/" << program_name << ".c";
      command << " -O3 -fPIC";
      command << " -shared";
      command << " -o "
              << "/tmp/" << program_name << ".so";
      std::system(command.str().c_str());
   }
   was_compiled = true;
}

BackendProgramC::~BackendProgramC() {
   // TODO dlclose
}

void* BackendProgramC::getFunction(std::string_view name) {
   if (!was_compiled) {
      throw std::runtime_error("Function has to be compiled before a handle can be returned");
   }
   if (!handle) {
      // Dlopen for the first time
      auto soname = "/tmp/" + program_name + ".so";
      handle = dlopen(soname.c_str(), RTLD_NOW);
   }
   if (!handle) {
      throw std::runtime_error("Error during dlopen: " + std::string(dlerror()));
   }

   auto fn = dlsym(handle, name.data());

   return fn;
}

void BackendProgramC::dump() {
   // Open the file.
   std::ofstream out("/tmp/" + program_name + ".c");
   // Write the program.
   out << program;
   // And close the stream.
   out.close();
}

std::unique_ptr<IR::BackendProgram> BackendC::generate(const IR::Program& program) const {
   ScopedWriter writer;

   // Step 1: Set up the preamble containing includes.
   createPreamble(writer);

   // Step 2: Set up the structs used throughout the program.
   for (const auto& structure : program.getStructs()) {
      compileStruct(*structure, writer);
   }

   // Step 3: Set up the functions used throughout the program.
   for (const auto& function : program.getFunctions()) {
      compileFunction(*function, writer);
   }

   return std::make_unique<BackendProgramC>(writer.str(), program.program_name);
}

void BackendC::createPreamble(ScopedWriter& writer) {
   // Include the integer types needed.
   writer.stmt(false).stream() << "#include <stdint.h>\n";
}

void BackendC::compileInclude(const IR::Program& include, ScopedWriter& writer) {
   // TODO
}

void BackendC::typeDescription(const IR::Type& type, ScopedWriter::Statement& str) {
   struct DescriptionVisitor final : public IR::TypeVisitor<ScopedWriter::Statement&> {
      private:
      void visitSignedInt(const IR::SignedInt& type, ScopedWriter::Statement& arg) override {
         arg.stream() << "int" << 8 * type.numBytes() << "_t";
      }

      void visitUnsignedInt(const IR::UnsignedInt& type, ScopedWriter::Statement& arg) override {
         arg.stream() << "uint" << 8 * type.numBytes() << "_t";
      }

      void visitBool(const IR::Bool& type, ScopedWriter::Statement& arg) override {
         arg.stream() << "bool";
      }
   };

   DescriptionVisitor visitor;
   visitor.visit(type, str);
}

void BackendC::compileStruct(const IR::Struct& structure, ScopedWriter& str) {
   // TODO
}

void BackendC::compileFunction(const IR::Function& fct, ScopedWriter& writer) {
   {
      // Set up the function definition.
      auto fct_decl = writer.stmt(!fct.getBody());
      typeDescription(*fct.return_type, fct_decl);
      fct_decl.stream() << " " << fct.name << "(";
      for (size_t k = 0; k < fct.arguments.size(); ++k) {
         const auto& arg = fct.arguments[k];
         const auto& decl = dynamic_cast<const IR::DeclareStmt&>(*arg);
         typeDescription(*decl.type, fct_decl);
         fct_decl.stream() << " " << decl.name;
         if (k != fct.arguments.size() - 1) {
            fct_decl.stream() << ", ";
         }
      }
      fct_decl.stream() << ")";
   }
   if (fct.getBody()) {
      // Definition is written, now compile the actual code.
      auto fct_block = writer.block();
      compileBlock(*fct.getBody(), writer);
   }
}

void BackendC::compileBlock(const IR::Block& block, ScopedWriter& writer) {
   // Simply compile one statement after the other.
   for (auto& stmt : block.statements) {
      compileStatement(*stmt, writer);
   }
}

void BackendC::compileStatement(const IR::Stmt& statement, ScopedWriter& writer) {
   struct StatementVisitor final : public IR::StmtVisitor<ScopedWriter::Statement&> {
      void visitDeclare(const IR::DeclareStmt& stmt, ScopedWriter::Statement& writer) override {
         typeDescription(*stmt.type, writer);
         writer.stream() << " " << stmt.name;
      }

      void visitAssignment(const IR::AssignmentStmt& stmt, ScopedWriter::Statement& writer) override {
         writer.stream() << stmt.declaraion.name << " = ";
         compileExpression(*stmt.expr, writer);
      }

      void visitReturn(const IR::ReturnStmt& stmt, ScopedWriter::Statement& writer) override {
         writer.stream() << "return ";
         compileExpression(*stmt.expr, writer);
      }
   };

   StatementVisitor visitor;
   auto stmt = writer.stmt();
   visitor.visit(statement, stmt);
}

void BackendC::compileExpression(const IR::Expr& expr, ScopedWriter::Statement& str) {
   // TODO clean up and turn into proper visitor.
   if (auto elem = dynamic_cast<const IR::VarRefExpr*>(&expr); elem) {
      str.stream() << elem->declaration.name;
   } else if (auto elem = dynamic_cast<const IR::ConstExpr*>(&expr); elem) {
      compileValue(*elem->value, str);
   } else if (auto elem = dynamic_cast<const IR::ArithmeticExpr*>(&expr); elem) {
      compileExpression(*elem->children[0], str);
      switch (elem->code) {
         case IR::ArithmeticExpr::Opcode::Add: {
            str.stream() << " + ";
            break;
         }
         case IR::ArithmeticExpr::Opcode::Subtract: {
            str.stream() << " - ";
            break;
         }
         default:
            throw std::runtime_error("arithmetic op in c backend not implemented");
      }
      compileExpression(*elem->children[1], str);
   } else {
      throw std::runtime_error("expression in c backend not implemented");
   }
}

void BackendC::compileValue(const IR::Value& value, ScopedWriter::Statement& str) {
   if (auto elem = dynamic_cast<const IR::UI<4>*>(&value); elem) {
      str.stream() << std::to_string(elem->value);
   } else if (auto elem = dynamic_cast<const IR::UI<8>*>(&value); elem) {
      str.stream() << std::to_string(elem->value);
   }
}

}
#include "codegen/backend_c/BackendC.h"
#include "exec/InterruptableJob.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <dlfcn.h>

namespace inkfuse {

namespace {

static constexpr bool debug_mode = false;

/// Generate the path for the c program.
std::string path(std::string_view program_name) {
   std::stringstream stream;
   stream << "/tmp/" << program_name << ".c";
   return stream.str();
}

/// Generate the path for the so.
std::string so_path(std::string_view program_name) {
   std::stringstream stream;
   stream << "/tmp/" << program_name << ".so";
   return stream.str();
}

}

void BackendProgramC::compileToMachinecode(InterruptableJob& interrupt) {
   if (!was_compiled) {
      // Dump.
      dump();
      // Invoke the compiler.
      std::stringstream command;
#ifdef WITH_JIT_CLANG_14
      command << "clang-14 ";
#else
      const char* env = std::getenv("CUSTOM_JIT");
      if (!env) {
         throw std::runtime_error("Custom compiler has to be set through CUSTOM_JIT env variable.");
      }
      command << env << " ";
#endif
      command << path(program_name);
      if constexpr (debug_mode) {
         command << " -g -O0 -fPIC";
      } else {
         command << " -g -O3 -fPIC";
      }
      command << " -shared";
      command << " -o ";
      command << so_path(program_name);
      auto command_str = command.str();

      auto exit_code = Command::runShell(command_str, interrupt);
      if (interrupt.getResult() == InterruptableJob::Change::Interrupted) {
         return;
      }
      if (exit_code != 0) {
         throw std::runtime_error("Compilation failed.");
      }

      // Add to compiled programs.
      backend->generated.insert(program_name);
   }
   was_compiled = true;
}

BackendProgramC::~BackendProgramC() {
   unlink();
}

void BackendProgramC::link() {
   if (!handle) {
      // Dlopen for the first time
      auto soname = so_path(program_name);
      handle = dlopen(soname.c_str(), RTLD_LOCAL | RTLD_LAZY);
      if (!handle) {
         fprintf(stderr, "dlopen failed: %s\n", dlerror());
         throw std::runtime_error("Could not link BackendProgramC.");
      }
   }
}

void BackendProgramC::unlink()
{
   if (handle) {
      dlclose(handle);
   }
}

void* BackendProgramC::getFunction(std::string_view name) {
   if (!was_compiled) {
      throw std::runtime_error("Function has to be compiled before a handle can be returned");
   }

   // Lazily link if it did not happen before.
   link();

   // Fetch the function.
   auto fn = dlsym(handle, name.data());

   return fn;
}

void BackendProgramC::dump() {
   // Open the file.
   std::ofstream out(path(program_name));
   // Write the program.
   out << program;
   // And close the stream.
   out.close();
   // Add to dumped programs.
   backend->dumped.insert(program_name);
}

std::unique_ptr<IR::BackendProgram> BackendC::generate(const IR::Program& program) {
   ScopedWriter writer;

   // Step 1: Set up the preamble.
   createPreamble(writer);

   // Step 2: Set up the includes.
   for (const auto& include : program.getIncludes()) {
      compileInclude(*include, writer);
   }

   // Step 3: Set up the structs used throughout the program.
   for (const auto& structure : program.getStructs()) {
      compileStruct(*structure, writer);
   }

   // Step 4: Set up the functions used throughout the program.
   for (const auto& function : program.getFunctions()) {
      compileFunction(*function, writer);
   }

   return std::make_unique<BackendProgramC>(*this, writer.str(), program.program_name);
}

void BackendC::createPreamble(ScopedWriter& writer) {
   // Include the integer types needed.
   writer.stmt(false).stream() << "#include <stdint.h>";
   // We need to access strcmp.
   writer.stmt(false).stream() << "#include <string.h>";
   writer.stmt(false).stream() << "#include <stdbool.h>\n";
}

void BackendC::compileInclude(const IR::Program& include, ScopedWriter& writer) {
   if (!dumped.count(include.program_name)) {
      // Include was not dumped yet - generate it.
      generate(include)->dump();
   }
   // Now we can just include the respective program.
   writer.stmt(false).stream() << "#include \"" << path(include.program_name) << "\"\n";
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

      void visitFloat(const IR::Float& type, ScopedWriter::Statement& arg) override {
         if (type.numBytes() == 4) {
            arg.stream() << "float";
         } else {
            arg.stream() << "double";
         }
      }

      void visitChar(const IR::Char& type, ScopedWriter::Statement& arg) override {
         arg.stream() << "char";
      }

      void visitString(const IR::String& type, ScopedWriter::Statement& arg) override {
         arg.stream() << "char*";
      }

      void visitDate(const IR::Date& type, ScopedWriter::Statement& arg) override {
         arg.stream() << "int32_t";
      }

      void visitBool(const IR::Bool& type, ScopedWriter::Statement& arg) override {
         arg.stream() << "bool";
      }

      void visitByteArray(const IR::ByteArray& /*type*/, ScopedWriter::Statement& arg) override {
         // A byte array in the generated C code is really just a char *.
         // The byte array hides behind this pointer. The generated code has custom
         // offsetting logic. Check IndexedIUProvider.
         arg.stream() << "char*";
      }

      void visitVoid(const IR::Void& /*type*/, ScopedWriter::Statement& arg) override {
         arg.stream() << "void";
      }

      void visitStruct(const IR::Struct& type, ScopedWriter::Statement& arg) override {
         arg.stream() << "struct " << type.name;
      }

      void visitPointer(const IR::Pointer& type, ScopedWriter::Statement& arg) override {
         typeDescription(*type.pointed_to, arg);
         arg.stream() << "*";
      }
   };

   DescriptionVisitor visitor;
   visitor.visit(type, str);
}

void BackendC::compileStruct(const IR::Struct& structure, ScopedWriter& writer) {
   {
      auto struct_def = writer.stmt(false);
      struct_def.stream() << "struct " << structure.name;
   }
   {
      // Struct body.
      auto block = writer.block();
      for (const auto& field : structure.fields) {
         auto field_def = writer.stmt();
         typeDescription(*field.type, field_def);
         field_def.stream() << " " << field.name;
      }
   }
   writer.stmt(true);
}

void BackendC::compileFunction(const IR::Function& fct, ScopedWriter& writer) {
   {
      // Set up the function definition.
      auto fct_decl = writer.stmt(!fct.getBody());
      if (!fct.getBody()) {
         // Extern function, will be linked later.
         fct_decl.stream() << "extern ";
      }
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
   writer.stmt(false);
}

void BackendC::compileBlock(const IR::Block& block, ScopedWriter& writer) {
   // Simply compile one statement after the other.
   for (auto& stmt : block.statements) {
      compileStatement(*stmt, writer);
   }
}

void BackendC::compileStatement(const IR::Stmt& statement, ScopedWriter& writer) {
   // Control flow statements open more than one statement.
   struct ControlFlowVisitor final : public IR::StmtVisitor<ScopedWriter&> {
      bool visitIf(const IR::IfStmt& stmt, ScopedWriter& writer) {
         {
            // Set up actual if statement.
            auto if_head = writer.stmt(false);
            if_head.stream() << "if (";
            compileExpression(*stmt.expr, if_head);
            if_head.stream() << ")";
         }
         {
            // Set up if block content.
            auto block = writer.block();
            compileBlock(*stmt.if_block, writer);
         }
         if (!stmt.else_block->statements.empty()) {
            // The else is not empty, set it up.
            writer.stmt(false).stream() << "else";
            auto block = writer.block();
            compileBlock(*stmt.else_block, writer);
         }
         return true;
      }

      bool visitWhile(const IR::WhileStmt& stmt, ScopedWriter& writer) {
         {
            // Set up actual while statement.
            auto while_head = writer.stmt(false);
            while_head.stream() << "while (";
            compileExpression(*stmt.expr, while_head);
            while_head.stream() << ")";
         }
         // Set up while block content.
         auto block = writer.block();
         compileBlock(*stmt.while_block, writer);
         return true;
      }
   };

   // Regular statements can meanwhile directly take a statement to write into.
   struct StatementVisitor final : public IR::StmtVisitor<ScopedWriter::Statement&> {

      bool visitDeclare(const IR::DeclareStmt& stmt, ScopedWriter::Statement& writer) override {
         typeDescription(*stmt.type, writer);
         writer.stream() << " " << stmt.name;
         return true;
      }

      bool visitInvokeFct(const IR::InvokeFctStmt& stmt, ScopedWriter::Statement& writer) override {
         compileExpression(*stmt.invoke_fct_expr, writer);
         return true;
      }

      bool visitAssignment(const IR::AssignmentStmt& stmt, ScopedWriter::Statement& writer) override {
         compileExpression(*stmt.left_side, writer);
         writer.stream() << " = ";
         compileExpression(*stmt.expr, writer);
         return true;
      }

      bool visitReturn(const IR::ReturnStmt& stmt, ScopedWriter::Statement& writer) override {
         writer.stream() << "return ";
         compileExpression(*stmt.expr, writer);
         return true;
      }
   };
   ControlFlowVisitor controlVisitor;
   if (!controlVisitor.visit(statement, writer)) {
      // This is not a control flow block, and we need to visit again for regular statements.
      StatementVisitor visitor;
      auto stmt = writer.stmt();
      visitor.visit(statement, stmt);
   }
}

void BackendC::compileExpression(const IR::Expr& expr, ScopedWriter::Statement& str) {
   // Visitor for the different expressions which have to be handled.
   struct ExpressionVisitor final : public IR::ExprVisitor<ScopedWriter::Statement&> {
      void visitInvokeFct(const IR::InvokeFctExpr& type, ScopedWriter::Statement& stmt) override {
         stmt.stream() << type.fct.name << "(";
         for (auto it = type.children.cbegin(); it != type.children.cend(); ++it) {
            if (it != type.children.cbegin()) {
               stmt.stream() << ", ";
            }
            compileExpression(**it, stmt);
         }
         stmt.stream() << ")";
      }

      void visitVarRef(const IR::VarRefExpr& type, ScopedWriter::Statement& stmt) override {
         stmt.stream() << type.declaration.name;
      }

      void visitConst(const IR::ConstExpr& type, ScopedWriter::Statement& stmt) override {
         compileValue(*type.value, stmt);
      }

      void visitArithmetic(const IR::ArithmeticExpr& type, ScopedWriter::Statement& stmt) override {
         static const std::unordered_map<IR::ArithmeticExpr::Opcode, std::string> opcode_map{
            {IR::ArithmeticExpr::Opcode::Add, "+"},
            {IR::ArithmeticExpr::Opcode::Subtract, "-"},
            {IR::ArithmeticExpr::Opcode::Multiply, "*"},
            {IR::ArithmeticExpr::Opcode::Divide, "/"},
            {IR::ArithmeticExpr::Opcode::Less, "<"},
            {IR::ArithmeticExpr::Opcode::LessEqual, "<="},
            {IR::ArithmeticExpr::Opcode::Greater, ">"},
            {IR::ArithmeticExpr::Opcode::GreaterEqual, ">="},
            {IR::ArithmeticExpr::Opcode::And, "&&"},
            {IR::ArithmeticExpr::Opcode::Or, "||"},
            {IR::ArithmeticExpr::Opcode::Eq, "=="},
            {IR::ArithmeticExpr::Opcode::Neq, "!="},
         };
         if (type.code == IR::ArithmeticExpr::Opcode::StrEquals) {
            // Strcmp needs to return 0 for two strings to be equal.
            stmt.stream() << "(strcmp(";
            compileExpression(*type.children[0], stmt);
            stmt.stream() << ", ";
            compileExpression(*type.children[1], stmt);
            stmt.stream() << ") == 0)";
         } else {
            // Regular arithmethic operation that's directly supported by C.
            assert(opcode_map.count(type.code));
            compileExpression(*type.children[0], stmt);
            stmt.stream() << " " << opcode_map.at(type.code) << " ";
            compileExpression(*type.children[1], stmt);
         }
      }

      void visitDeref(const IR::DerefExpr& type, ScopedWriter::Statement& stmt) override {
         stmt.stream() << "(*(";
         compileExpression(*type.children[0], stmt);
         stmt.stream() << "))";
      }

      void visitRef(const IR::RefExpr& type, ScopedWriter::Statement& stmt) override {
         stmt.stream() << "(&(";
         compileExpression(*type.children[0], stmt);
         stmt.stream() << "))";
      }

      void visitStructAccess(const IR::StructAccessExpr& type, ScopedWriter::Statement& stmt) override {
         compileExpression(*type.children[0], stmt);
         stmt.stream() << "." << type.field;
      }

      void visitCast(const IR::CastExpr& type, ScopedWriter::Statement& stmt) override {
         // Set up cast into target type.
         stmt.stream() << "((";
         typeDescription(*type.type, stmt);
         stmt.stream() << ") (";
         // Set up what should be casted.
         compileExpression(*type.children[0], stmt);
         stmt.stream() << "))";
      }
   };

   ExpressionVisitor visitor;
   visitor.visit(expr, str);
}

void BackendC::compileValue(const IR::Value& value, ScopedWriter::Statement& str) {
   str.stream() << value.str();
}

}

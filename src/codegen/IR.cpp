#include "codegen/IR.h"
#include "runtime/Runtime.h"

namespace inkfuse {

namespace IR {

Function::Function(std::string name_, std::vector<StmtPtr> arguments_, TypeArc return_type_)
   : name(std::move(name_)), arguments(std::move(arguments_)), return_type(std::move(return_type_)) {
   for (const auto& ptr : arguments) {
      if (!dynamic_cast<DeclareStmt*>(ptr.get())) {
         throw std::runtime_error("function must have declare statements as arguments");
      }
   }
}

const Block* Function::getBody() const {
   return body.get();
}

void Block::appendStmt(StmtPtr stmt) {
   statements.push_back(std::move(stmt));
}

void Block::appendStmts(std::deque<StmtPtr> stmts) {
   statements.insert(
      statements.end(),
      std::make_move_iterator(stmts.begin()),
      std::make_move_iterator(stmts.end())
      );
}

void Block::prependStmts(StmtPtr stmt) {
   statements.push_front(std::move(stmt));
}

void Block::prependStmts(std::deque<StmtPtr> stmts) {
   statements.insert(
      statements.begin(),
      std::make_move_iterator(stmts.begin()),
      std::make_move_iterator(stmts.end())
   );
}

Program::Program(std::string program_name_, bool standalone) : program_name(std::move(program_name_)) {
   if (!standalone) {
      // We depend on the global runtime by default.
      includes.push_back(global_runtime.program);
   }
}

IRBuilder Program::getIRBuilder() {
   return IRBuilder(*this);
}

const std::vector<ProgramArc>& Program::getIncludes() const {
   return includes;
}

const std::vector<StructArc>& Program::getStructs() const {
   return structs;
}

const std::vector<FunctionArc>& Program::getFunctions() const {
   return functions;
}

TypeArc Program::getStruct(std::string_view name) const {
   for (const auto& str : structs) {
      if (str->name == name) {
         return str;
      }
   }
   for (const auto& include : includes) {
      if (auto res = include->getStruct(name); res) {
         return res;
      }
   }
   throw std::runtime_error("struct does not exist");
}

FunctionArc Program::getFunction(std::string_view name) const {
   for (const auto& fct : functions) {
      if (fct->name == name) {
         return fct;
      }
   }
   for (const auto& include : includes) {
      if (auto res = include->getFunction(name)) {
         return res;
      }
   }
   throw std::runtime_error("function does not exist");
}

}

}

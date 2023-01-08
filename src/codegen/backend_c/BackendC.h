#ifndef INKFUSE_BACKENDC_H
#define INKFUSE_BACKENDC_H

#include "codegen/Backend.h"
#include "codegen/Type.h"
#include "codegen/backend_c/ScopedWriter.h"
#include <cstdint>
#include <memory>
#include <sstream>
#include <unordered_set>
#include <string>

namespace inkfuse {

struct BackendC;

/// Backend program in C.
struct BackendProgramC : public IR::BackendProgram {
   BackendProgramC(BackendC& backend_, std::string program_, std::string program_name_) : backend(&backend_), program(std::move(program_)), program_name(std::move(program_name_)){};

   ~BackendProgramC() override;

   /// Link the backend program. This way, it doesn't get linked lazily during the first lookup.
   void link() override;

   /// Unlink the backend program.
   void unlink() override;

   /// Compile the backend program to actual machine code. The interrupt is used to stop compilation.
   void compileToMachinecode(InterruptableJob& interrupt) override;

   /// Get a function with the specified name from the compiled program.
   void* getFunction(std::string_view name) override;

   /// Dump the backend program in a readable way into a file.
   void dump() override;

   private:
   /// Backend from which this program was generated.
   BackendC* backend;
   /// Was this program compiled already?
   bool was_compiled = false;
   /// The actual program which is simply generated C stored within a backing string.
   const std::string program;
   /// The program name.
   const std::string program_name;
   /// Handle to the dlopened so.
   void* handle = nullptr;
};

/// Simple C backend translating the inkfuse IR to plain C.
struct BackendC : public IR::Backend {
   ~BackendC() override = default;

   /// Generate a backend program from the high-level IR.
   std::unique_ptr<IR::BackendProgram> generate(const IR::Program& program) override;

   private:
   /// Compile an included other program.
   void compileInclude(const IR::Program& include, ScopedWriter& writer);

   /// Set up the preamble.
   static void createPreamble(ScopedWriter& writer);

   /// Add a type description to the backing string stream.
   static void typeDescription(const IR::Type& type, ScopedWriter::Statement& writer);

   /// Compile a structure.
   static void compileStruct(const IR::Struct& structure, ScopedWriter& writer);

   /// Compile a function.
   static void compileFunction(const IR::Function& fct, ScopedWriter& writer);

   /// Compile a block.
   static void compileBlock(const IR::Block& block, ScopedWriter& writer);

   /// Compile a statement.
   static void compileStatement(const IR::Stmt& statement, ScopedWriter& writer);

   /// Compile an expression.
   static void compileExpression(const IR::Expr& expr, ScopedWriter::Statement& str);

   /// Compile a value.
   static void compileValue(const IR::Value& value, ScopedWriter::Statement& str);

   /// For which programs was the C dumped already? Can be used for includes.
   std::unordered_set<std::string> dumped;
   /// For which programs was code generated already?
   std::unordered_set<std::string> generated;

   friend class BackendProgramC;
};

}

#endif //INKFUSE_BACKENDC_H

#ifndef INKFUSE_RUNTIMEPARAMS_H
#define INKFUSE_RUNTIMEPARAMS_H

#include "algebra/CompilationContext.h"
#include "algebra/suboperators/Suboperator.h"
#include "codegen/Expression.h"
#include "codegen/Type.h"
#include "codegen/Value.h"
#include <memory>
#include <optional>

namespace inkfuse {

/// A RuntimeParam is a parameter influencing suboperator behaviour
/// in a way that cannot always be baked into the compiled code.
/// Take a simple example: addition with a constant. During compiled evaluation,
/// we want to bake the constant into the generated code.
/// During interpreted interpretation meanwhile, we have to take the value from the
/// operator global state.
/// This is allowed through the RUNTIME_PARAM macro. While it might seem unergonimic at first,
/// it allows for static typing of these runtime parameters.
#ifndef RUNTIME_PARAM

#define RUNTIME_PARAM(pname, gstate)                                                                                  \
   IR::ValuePtr pname;                                                                                             \
                                                                                                                   \
   void pname##Set(IR::ValuePtr new_val) {                                                                         \
      pname = std::move(new_val);                                                                                  \
   }                                                                                                               \
                                                                                                                   \
   IR::ExprPtr pname##Resolve(const Suboperator& op, CompilationContext& context) const {                          \
      if (!pname) {                                                                                                 \
         const auto& program = context.getProgram();                                                               \
         auto state_expr = context.accessGlobalState(op);                                                          \
         auto casted_state = IR::CastExpr::build(                                                                  \
            std::move(state_expr),                                                                                 \
            IR::Pointer::build(program.getStruct(gstate::name)));                                                  \
         /* As there is no type erasure, get the struct member directly value and deref. */                                                                                                          \
         return IR::StructAccessExpr::build(std::move(casted_state), #pname);                                      \
      } else {                                                                                                     \
         return IR::ConstExpr::build(pname->copy());                                                               \
      }                                                                                                            \
   }                                                                                                               \
                                                                                                                   \
   IR::ExprPtr pname##ResolveErased(const Suboperator& op, const IR::TypeArc& type, CompilationContext& context) const { \
      if (!pname) {                                                                                                 \
         const auto& program = context.getProgram();                                                               \
         auto state_expr = context.accessGlobalState(op);                                                          \
         auto casted_state = IR::CastExpr::build(                                                                  \
            std::move(state_expr),                                                                                 \
            IR::Pointer::build(program.getStruct(gstate::name)));                                                  \
         /* Get the void pointer to the erased type, cast as pointer to value and deref. */                                                                                                          \
         return IR::DerefExpr::build(                                                                              \
            IR::CastExpr::build(                                                                                   \
               IR::StructAccessExpr::build(std::move(casted_state), #pname "_erased"),                             \
               IR::Pointer::build(type)));                                                                         \
      } else {                                                                                                     \
         return IR::ConstExpr::build(pname->copy());                                                               \
      }                                                                                                            \
   }

#endif
}

#endif //INKFUSE_RUNTIMEPARAMS_H

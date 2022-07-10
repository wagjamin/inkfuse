#include "algebra/suboperators/expressions/RuntimeExpressionSubop.h"
#include "codegen/Type.h"
#include "codegen/Value.h"
#include "gtest/gtest.h"

namespace inkfuse {

namespace {

// Test the runtime parameter code generation with a runtime param that gets loaded at runtime.
TEST(test_runtime_param, runtime_expression_no_val) {
   Pipeline pipe;
   CompilationContext context("runtime_param_test", pipe);
   IU source(IR::UnsignedInt::build(4));
   IU provided(IR::UnsignedInt::build(4));
   auto l_expr = RuntimeExpressionSubop::build(nullptr, {&provided}, {&source}, ExpressionOp::ComputeNode::Type::Add, IR::UnsignedInt::build(4));
   pipe.attachSuboperator(l_expr);
   pipe.repipeAll(0, 1);
   EXPECT_NO_THROW(context.compile());
}

// Test the runtime parameter code generation with a runtime param that gets statically substituted during compile time.
TEST(test_runtime_param, runtime_expression_val) {
   RuntimeExpressionParams params;
   params.dataSet(IR::UI<4>::build(4));
   Pipeline pipe;
   CompilationContext context("runtime_param_test", pipe);
   IU source(IR::UnsignedInt::build(4));
   IU provided(IR::UnsignedInt::build(4));
   auto l_expr = RuntimeExpressionSubop::build(nullptr, {&provided}, {&source}, ExpressionOp::ComputeNode::Type::Add, IR::UnsignedInt::build(4));
   static_cast<RuntimeExpressionSubop&>(*l_expr).attachRuntimeParams(std::move(params));
   pipe.attachSuboperator(l_expr);
   pipe.repipeAll(0, 1);
   EXPECT_NO_THROW(context.compile());
}

}

}
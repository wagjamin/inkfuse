#include "algebra/suboperators/expressions/LazyExpressionSubop.h"
#include "codegen/Type.h"
#include "codegen/Value.h"
#include "gtest/gtest.h"

namespace inkfuse {

namespace {

// Test the lazy parameter code generation with a lazy param that gets loaded at runtime.
TEST(test_lazy_param, lazy_expression_no_val) {
   Pipeline pipe;
   CompilationContext context("lazy_param_test", pipe);
   IU source(IR::UnsignedInt::build(4));
   IU provided(IR::UnsignedInt::build(4));
   auto l_expr = LazyExpressionSubop::build(nullptr, {&provided}, {&source}, ExpressionOp::ComputeNode::Type::Add, IR::UnsignedInt::build(4));
   pipe.attachSuboperator(l_expr);
   pipe.repipeAll(0, 1);
   EXPECT_NO_THROW(context.compile());
}

// Test the lazy parameter code generation with a lazy param that gets statically substituted during compile time.
TEST(test_lazy_param, lazy_expression_val) {
   LazyExpressionParams params;
   params.dataSet(IR::UI<4>::build(4));
   Pipeline pipe;
   CompilationContext context("lazy_param_test", pipe);
   IU source(IR::UnsignedInt::build(4));
   IU provided(IR::UnsignedInt::build(4));
   auto l_expr = LazyExpressionSubop::build(nullptr, {&provided}, {&source}, ExpressionOp::ComputeNode::Type::Add, IR::UnsignedInt::build(4));
   static_cast<LazyExpressionSubop&>(*l_expr).attachLazyParams(std::move(params));
   pipe.attachSuboperator(l_expr);
   pipe.repipeAll(0, 1);
   EXPECT_NO_THROW(context.compile());
}

}

}
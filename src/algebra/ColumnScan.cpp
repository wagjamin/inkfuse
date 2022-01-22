#include "algebra/ColumnScan.h"
#include "exec/FuseChunk.h"
#include "runtime/Runtime.h"

namespace inkfuse {

ColumnScan::ColumnScan(ColumnScanParameters params_, IURef provided_iu_, const BaseColumn& column_)
   : params(std::move(params_)), provided_iu(provided_iu_), column(column_) {
}

void ColumnScan::compile(CompilationContext& context, const std::set<IURef>& required) const {
}

void ColumnScan::setUpState() {
   state = std::make_unique<ColumnScanState>(ColumnScanState{
      .data = column.getRawData(),
      .size = column.length(),
   });
}

void ColumnScan::tearDownState() {
   state.reset();
}

void * ColumnScan::accessState() const
{
    return state.get();
}

std::string ColumnScan::id() const {
   /// Only have to parametrize over the type.
   return "column_scan_" + params.type->id();
}

void ColumnScan::registerRuntime()
{
    /// ColumnScanState needs to be accessible from within the generated code.
    RuntimeStructBuilder("ColumnScanState")
        .addMember("data", IR::Pointer::build(IR::Void::build()))
        .addMember("sel", IR::Pointer::build(IR::Bool::build()))
        .addMember("size", IR::UnsignedInt::build(8));

}

}

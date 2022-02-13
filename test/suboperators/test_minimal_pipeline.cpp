#include <gtest/gtest.h>
#include "algebra/CompilationContext.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/sources/TableScanSource.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "codegen/backend_c/BackendC.h"

namespace inkfuse {

namespace {

/// Empty pipeline test.
TEST(test_minimal_pipeline, pipeline_1) {
   Pipeline pipe;
   CompilationContext codegen("test_minimal_pipeline_pipeline_1", pipe);
   codegen.compile();

   // Generate code.
   BackendC backend;
   auto program = backend.generate(codegen.getProgram());
   program->dump();
}

/// Table scan with explicit fuse chunk sink.
TEST(test_minimal_pipeline, pipeline_2) {
   Pipeline pipe;
   CompilationContext codegen("test_minimal_pipeline_pipeline_2", pipe);

   // Read from storage.
   IU scan_iu_1(IR::UnsignedInt::build(8), "scan_1");
   IU scan_iu_2(IR::UnsignedInt::build(4), "scan_2");

   // Loop driver.
   auto tscan_driver = TScanDriver::build(nullptr);
   auto& driver_iu = **tscan_driver->getIUs().begin();
   // IU providers.
   auto provider_1 = TScanIUProvider::build(nullptr, driver_iu, scan_iu_1);
   auto provider_2 = TScanIUProvider::build(nullptr, driver_iu, scan_iu_2);
   // Sinks.
   auto sink_1 = FuseChunkSink::build(nullptr, scan_iu_1);
   auto sink_2 = FuseChunkSink::build(nullptr, scan_iu_2);

   pipe.attachSuboperator(std::move(tscan_driver));
   pipe.attachSuboperator(std::move(provider_1));
   pipe.attachSuboperator(std::move(provider_2));
   pipe.attachSuboperator(std::move(sink_1));
   pipe.attachSuboperator(std::move(sink_2));

   // Compile.
   codegen.compile();

   // Generate C code.
   BackendC backend;
   auto program = backend.generate(codegen.getProgram());
   program->dump();
   program->compileToMachinecode();
}

/// Table scan with implicit fuse chunk sink.
TEST(test_minimal_pipeline, pipeline_3) {
   Pipeline pipe;

   // Read from storage.
   IU scan_iu_1(IR::UnsignedInt::build(8), "scan_1");
   IU scan_iu_2(IR::UnsignedInt::build(4), "scan_2");

   // Loop driver.
   auto tscan_driver = TScanDriver::build(nullptr);
   auto& driver_iu = **tscan_driver->getIUs().begin();
   // IU providers.
   auto provider_1 = TScanIUProvider::build(nullptr, driver_iu, scan_iu_1);
   auto provider_2 = TScanIUProvider::build(nullptr, driver_iu, scan_iu_2);
   pipe.attachSuboperator(std::move(tscan_driver));
   pipe.attachSuboperator(std::move(provider_1));
   pipe.attachSuboperator(std::move(provider_2));

   // Sinks should get auto-generated.
   auto repiped = pipe.repipe(0, 3);

   // Compile.
   CompilationContext codegen("test_minimal_pipeline_pipeline_3", *repiped);
   codegen.compile();

   // Generate C code.
   BackendC backend;
   auto program = backend.generate(codegen.getProgram());
   program->dump();
   program->compileToMachinecode();
}

}

}

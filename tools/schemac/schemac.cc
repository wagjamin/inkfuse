#include <iostream>
#include <fstream>

#include "imlab/schemac/schema_parse_context.h"
#include "imlab/schemac/schema_compiler.h"
#include "gflags/gflags.h"

using SchemaCompiler = imlab::schemac::SchemaCompiler;
using SchemaParseContext = imlab::schemac::SchemaParseContext;

DEFINE_string(out_cc, "", "Output directory");
DEFINE_string(out_h, "", "Output directory");
DEFINE_string(in, "" , "SQL schema");

static bool ValidateWritable(const char *flagname, const std::string &value) {
    std::ofstream out(value);
    return out.good();
}
static bool ValidateReadable(const char *flagname, const std::string &value) {
    std::ifstream in(value);
    return in.good();
}

DEFINE_validator(out_cc, &ValidateWritable);
DEFINE_validator(out_h, &ValidateWritable);
DEFINE_validator(in, &ValidateReadable);

int main(int argc, char *argv[]) {
    gflags::SetUsageMessage("schemac --in <SCHEMA> --out_cc <H> --out_h <CC>");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::ifstream in(FLAGS_in);
    std::ofstream out_h(FLAGS_out_h, std::ofstream::trunc);
    std::ofstream out_cc(FLAGS_out_cc, std::ofstream::trunc);

    SchemaParseContext parse_context;
    auto schema = parse_context.Parse(in);

    SchemaCompiler compiler(out_h, out_cc, "gen/tpcc.h");
    compiler.Compile(schema);
}

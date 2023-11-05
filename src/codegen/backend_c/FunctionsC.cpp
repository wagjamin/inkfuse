#include "BackendC.h"
namespace inkfuse {

/// C code that's dumped into global_runtime.h during initial runtime
/// code generation.
char* BackendC::runtime_functions = R"PRE(
struct InLiteralList {
    const char** start;
    uint64_t size;
};

bool in_strlist(const char* strlist, char* arg) {
    const struct InLiteralList* list = (const struct InLiteralList*) strlist;
    bool res = false;
    for (uint64_t k = 0; k < list->size; ++k) {
        res |= (strcmp(list->start[k], arg) == 0);
    }
    return res;
}
)PRE";

};

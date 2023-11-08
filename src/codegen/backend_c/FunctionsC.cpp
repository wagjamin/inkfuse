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

bool not_like_tokens(const char* strlist, char* arg) {
    const struct InLiteralList* literal_list = (const struct InLiteralList*) strlist;

    // Iterate through each token in the literal list
    for (uint64_t i = 0; i < literal_list->size; i++) {
        const char* token = literal_list->start[i];

        // Check if the token is found in the argument
        char* token_in_arg = strstr(arg, token);

        // If the token is not found in the argument, return true 
        if (token_in_arg == NULL) {
            return true;
        }

        // Move the argument pointer to the character after the token
        arg = token_in_arg + strlen(token);
    }

    // If all tokens were found in the argument, return false 
    return false;
}
)PRE";
};

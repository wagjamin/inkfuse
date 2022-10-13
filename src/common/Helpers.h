#include <cstdint>
#include <string>

/// Common helper functions used throuhgout different parts of InkFuse.
namespace inkfuse::helpers
{

/// Transform a date literal into an integer represented in the runtime.
/// The date is stored as offset to the epoch 1-1-1970.
int32_t dateStrToInt(std::string& str);
    
} // namespace inkfuse

#ifndef INKFUSE_FUSECHUNK_H
#define INKFUSE_FUSECHUNK_H

#include <codegen/Types.h>

namespace inkfuse {

    /// A column within a FuseChunk.
    struct Column {
        Column(size_t capacity);

        /// Raw data stored within the column.
        char* raw_data;
        /// Capacity of the column.
        uint64_t capacity;
        /// Size of the column.
        uint64_t size;
    };

    /// A FuseChunk
    struct FuseChunk {

    };

}

#endif //INKFUSE_FUSECHUNK_H

#ifndef INKFUSE_IU_H
#define INKFUSE_IU_H

#include <string>
#include "codegen/Types.h"

namespace inkfuse {

    /// An information unit of a backing type. What's an information unit? Effectively it's an addressable value
    /// flowing through a query. For example, a table scan will define IUs for each of the columns it reads.
    /// As it iterates through its rows, the IUs represent the column value for each row.
    /// The IU is at the heart of pipeline-fusing engines and allows us to reference the addressable columns within
    /// the rows flowing throughout the engine.
    struct IU {
    public:

        /// Constructor.
        IU(IR::TypeArc type_, std::string name_ = ""): type(std::move(type_)), name(std::move(name_)) {};

        /// Type of this IU.
        IR::TypeArc type;
        /// Human-readable name for this IU.
        std::string name;
    };

    /// Reference to an IU so that it can be easily stored in collections.
    using IURef = std::reference_wrapper<IU>;

}


#endif //INKFUSE_IU_H

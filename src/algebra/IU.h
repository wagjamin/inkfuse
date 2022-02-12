#ifndef INKFUSE_IU_H
#define INKFUSE_IU_H

#include "codegen/Type.h"
#include <string>
#include <sstream>

namespace inkfuse {

/// An information unit of a backing type. What's an information unit? Effectively it's an addressable value
/// flowing through a query. For example, a table scan will define IUs for each of the columns it reads.
/// As it iterates through its rows, the IUs represent the column value for each row.
/// The IU is at the heart of pipeline-fusing engines and allows us to reference the addressable columns within
/// the rows flowing throughout the engine.
struct IU {
   public:
   /// Constructor.
   IU(IR::TypeArc type_, std::string name_) : type(std::move(type_)), name(std::move(name_)){};

   explicit IU(IR::TypeArc type_) : type(std::move(type_)) {
      const void * ptr = static_cast<const void*>(this);
      std::stringstream ss;
      ss << "iu_" << ptr;
      name = ss.str();
   };

   /// Type of this IU.
   IR::TypeArc type;
   /// The name for this IU.
   std::string name;
};

}

#endif //INKFUSE_IU_H

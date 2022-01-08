#include "codegen/Type.h"
#include <algorithm>

namespace inkfuse {

namespace IR {

    std::unique_ptr<Type> Type::max(const Type &t1, const Type &t2)
    {
        if (t1.isConst<UnsignedInt>() && t2.isConst<UnsignedInt>()) {
            return std::make_unique<UnsignedInt>(std::max(t1.isConst<UnsignedInt>()->numBytes(), t2.isConst<UnsignedInt>()->numBytes()));
        } else if(t1.isConst<SignedInt>() && t2.isConst<SignedInt>()) {
            return std::make_unique<UnsignedInt>(std::max(t1.isConst<UnsignedInt>()->numBytes(), t2.isConst<UnsignedInt>()->numBytes()));
        } else if (t1.isConst<SignedInt>() && t2.isConst<UnsignedInt>()) {

        } else if (t1.isConst<Bool>()) {
            return t2;
        }
    }

}

}

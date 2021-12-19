#include "imlab/algebra/iu.h"
#include <sstream>
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
std::string IU::Varname() const
{
    std::stringstream out;
    out << "iu_";
    out << this;
    return out.str();
}
// ---------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------

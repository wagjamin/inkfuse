#ifndef INKFUSE_IRHELPERS_H
#define INKFUSE_IRHELPERS_H

/// This file contains helper classes for the inkfuse IR.
namespace inkfuse {

namespace IR {

    /// IR Helper Class giving a few convenience functions.
    struct IRConcept {
    public:
        /// Is this a certain IR type?
        template <typename Other>
        Other* is() const {
            return dynamic_cast<Other*>(this);
        }

        /// Is this a certain IR type?
        template <typename Other>
        const Other* isConst() const {
            return dynamic_cast<const Other*>(this);
        }

    };

}

}

#endif //INKFUSE_IRHELPERS_H

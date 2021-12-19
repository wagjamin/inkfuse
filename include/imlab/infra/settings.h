//
// Created by benjamin-fire on 30.11.21.
//

#ifndef IMLAB_SETTINGS_H
#define IMLAB_SETTINGS_H

namespace imlab {

    // Global set of settings.
    struct Settings {
        // Generate code using tbb.
        static const bool USE_TBB = true;
        // Should we use the lazy multimap in joins?
        static const bool CODEGEN_LAZY_MULTIMAP = true | USE_TBB;
    };

} // namespace imlab

#endif //IMLAB_SETTINGS_H

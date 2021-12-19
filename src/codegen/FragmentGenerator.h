#ifndef INKFUSE_FRAGMENTGENERATOR_H
#define INKFUSE_FRAGMENTGENERATOR_H

namespace inkfuse {

    /// The FragmentGenerator is responsible for generating vectorized, pre-compiled fragments.
    /// During the compilation of the initial binary, these are generated and put into a shared object file.
    /// This object file can then be dynamically opened on inkfuse startup. It is later used by the FragmentCache
    /// to quickly provide the QueryExecutor access to the vectorized fragments during interpretation.
    struct FragmentGenerator {

    };

} // namespace inkfuse

#endif //INKFUSE_FRAGMENTGENERATOR_H

#ifndef INKFUSE_SCOPEDWRITER_H
#define INKFUSE_SCOPEDWRITER_H

#include <string>
#include <sstream>
#include <cstdint>

namespace inkfuse {

    /// Scoped writer for creating well-formatted C code.
    struct ScopedWriter {
    public:
        /// Scoped block. Constructor creates scope, destructor drops it.
        struct ScopedBlock {
        public:
            /// Destructor. Drops indent level.
            ~ScopedBlock();

        private:
            /// Constructor. Adds braces and new indent level.
            ScopedBlock(ScopedWriter& writer_);

            /// Backing writer.
            ScopedWriter& writer;

            friend class ScopedWriter;
        };

        /// Statement. Constructor sets up indentation, destructor clears it.
        struct Statement {
        public:
            /// Destructor. Terminates statement.
            ~Statement();

            /// Get backing stream of the statement.
            std::stringstream& stream();

        private:
            Statement(ScopedWriter& writer_, bool semicolon);

            /// Backing writer.
            ScopedWriter& writer;

            /// Should the statement be terminated by a semicolon:
            bool semicolon;

            friend class ScopedWriter;
        };

        /// Get backing string.
        std::string str() const;

        /// Create a new scoped block.
        ScopedBlock block();

        /// Create a new statement. Returns an r-value-reference, because it should never be stored locally and only
        /// used for chaining in the stream operator.
        /// @param semicolon indicates whether the statement should be terminated by a semicolon.
        Statement stmt(bool semicolon = true);

    private:
        /// Add indentation.
        void indent();
        /// Remove indentation.
        void unindent();

        /// Backing stream into which we write.
        std::stringstream stream;
        /// Current indentation level.
        size_t indent_level = 0;
        /// Current indentation string.
        std::string indent_string;

        friend class Statement;
        friend class ScopedBlock;
    };

}

#endif //INKFUSE_SCOPEDWRITER_H

include(ExternalProject)
find_package(Git REQUIRED)

# We use xxhash for fast hashing within e.g. the hash aggregation or hash join operators.
ExternalProject_Add(
        xxhash_src
        PREFIX "vendor/xxhash"
        GIT_REPOSITORY "https://github.com/Cyan4973/xxHash.git"
        INSTALL_DIR "vendor/xxhash/src/xxhash_src"
        BINARY_DIR "vendor/xxhash/src/xxhash_src"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND "$(MAKE)"
        INSTALL_COMMAND ""
        GIT_TAG v0.8.1
        TIMEOUT 10
)

# Prepare xxhash
ExternalProject_Get_Property(xxhash_src install_dir)
set(XXHASH_INCLUDE_DIR ${install_dir})
set(XXHASH_LIBRARY_PATH ${install_dir}/libxxhash.a)
add_library(xxhash_static STATIC IMPORTED)
set_property(TARGET xxhash_static PROPERTY IMPORTED_LOCATION ${XXHASH_LIBRARY_PATH})
set_property(TARGET xxhash_static PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${XXHASH_INCLUDE_DIR})
add_dependencies(xxhash_static xxhash_src)

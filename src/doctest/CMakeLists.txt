# CMake configuration for doctest-based tests
# These are converted from the beast unit_test framework

find_package(doctest REQUIRED)

# Custom target for all doctest tests defined in this file
add_custom_target(xrpl.doctests)

# Common library dependencies
add_library(xrpl.imports.doctest INTERFACE)
target_link_libraries(xrpl.imports.doctest INTERFACE
    doctest::doctest
    xrpl.libxrpl
)

# Include xrpld sources for tests that need app-level functionality
target_include_directories(xrpl.imports.doctest INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)

# Link against xrpld libraries for tests that need app-level functionality
# Tests like LoadFeeTrack, PendingSaves need xrpld components
target_link_libraries(xrpl.imports.doctest INTERFACE
    Xrpl::boost
    Xrpl::opts
    Xrpl::libs
)

# Collect all doctest source files
file(GLOB_RECURSE doctest_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Create the main test executable
add_executable(xrpl.doctest ${doctest_sources})
target_link_libraries(xrpl.doctest PRIVATE xrpl.imports.doctest)

# Make sure the test isn't optimized away in unity builds
set_target_properties(xrpl.doctest PROPERTIES
    UNITY_BUILD_MODE GROUP
    UNITY_BUILD_BATCH_SIZE 0
)

# Add as a CTest test
add_test(NAME xrpl.doctest COMMAND xrpl.doctest)

add_dependencies(xrpl.doctests xrpl.doctest)


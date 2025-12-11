# CMake configuration for doctest-based tests
# These are converted from the beast unit_test framework

include(XrplAddTest)

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
target_link_libraries(xrpl.imports.doctest INTERFACE
    Xrpl::boost
    Xrpl::opts
    Xrpl::libs
)

# Compiler flags for strict checking
set(DOCTEST_COMPILE_FLAGS
    -m64
    -g
    -fPIE
    -Wno-unknown-warning-option
    -Wall
    -Wdeprecated
    -Wno-deprecated-declarations
    -Wextra
    -Wno-unused-parameter
    -Werror
    -fstack-protector
    -Wno-sign-compare
    -Wno-unused-but-set-variable
)

# Helper function to add a doctest module
function(xrpl_add_doctest name)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${name}/*.cpp"
    )

    set(target xrpl.doctest.${name})
    add_executable(${target} ${sources})
    target_link_libraries(${target} PRIVATE xrpl.imports.doctest)
    target_compile_options(${target} PRIVATE ${DOCTEST_COMPILE_FLAGS})

    set_target_properties(${target} PROPERTIES
        UNITY_BUILD_MODE GROUP
        UNITY_BUILD_BATCH_SIZE 0
    )

    add_test(NAME ${target} COMMAND ${target})
    add_dependencies(xrpl.doctests ${target})
endfunction()

# One test executable for each module
xrpl_add_doctest(basics)
xrpl_add_doctest(beast)
xrpl_add_doctest(core)
xrpl_add_doctest(csf)
xrpl_add_doctest(nodestore)
xrpl_add_doctest(protocol)

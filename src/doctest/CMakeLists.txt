include(XrplAddTest)

# Test requirements.
find_package(doctest REQUIRED)

# Custom target for all doctest tests
add_custom_target(xrpl.doctest.tests)

# Common library dependencies for doctest tests
add_library(xrpl.imports.doctest INTERFACE)
target_link_libraries(xrpl.imports.doctest INTERFACE
    doctest::doctest
    xrpl.libxrpl
)

# Add test directories
# Note: These tests are converted from Beast Unit Test framework to doctest

# Basics tests (successfully migrated unit tests)
# Files: Buffer, Expected, IOUAmount, Number, XRPAmount
file(GLOB basics_test_sources basics/*_test.cpp)
add_executable(xrpl.test.basics main.cpp ${basics_test_sources})
target_link_libraries(xrpl.test.basics PRIVATE xrpl.libxrpl xrpl.imports.doctest)
add_dependencies(xrpl.doctest.tests xrpl.test.basics)


# Protocol tests (successfully migrated unit tests)
# Files: ApiVersion, BuildInfo, STAccount, STInteger, STNumber, SecretKey, Seed
file(GLOB protocol_test_sources protocol/*_test.cpp)
add_executable(xrpl.test.protocol main.cpp ${protocol_test_sources})
target_link_libraries(xrpl.test.protocol PRIVATE xrpl.libxrpl xrpl.imports.doctest)
add_dependencies(xrpl.doctest.tests xrpl.test.protocol)

# JSON tests (successfully migrated)
# File: Object
xrpl_add_test(json)
target_link_libraries(xrpl.test.json PRIVATE xrpl.imports.doctest)
add_dependencies(xrpl.doctest.tests xrpl.test.json)

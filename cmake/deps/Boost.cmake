include(CompilationEnv)
include(XrplSanitizers)

find_package(Boost REQUIRED
             COMPONENTS chrono
                        container
                        context
                        date_time
                        filesystem
                        json
                        program_options
                        regex
                        thread)

add_library(xrpl_boost INTERFACE)
add_library(Xrpl::boost ALIAS xrpl_boost)

target_link_libraries(
    xrpl_boost
    INTERFACE Boost::headers
              Boost::chrono
              Boost::container
              Boost::context
              Boost::date_time
              Boost::filesystem
              Boost::json
              Boost::process
              Boost::program_options
              Boost::regex
              Boost::thread)
if (Boost_COMPILER)
    target_link_libraries(xrpl_boost INTERFACE Boost::disable_autolinking)
endif ()

# Boost.Context's ucontext backend has ASAN fiber-switching annotations
# (start/finish_switch_fiber) that are compiled in when BOOST_USE_ASAN is defined.
# This tells ASAN about coroutine stack switches, preventing false positive
# stack-use-after-scope errors. BOOST_USE_UCONTEXT ensures the ucontext backend
# is selected (fcontext does not support ASAN annotations).
# These defines must match what Boost was compiled with (see conan/profiles/sanitizers).
if (enable_asan)
    target_compile_definitions(xrpl_boost INTERFACE BOOST_USE_ASAN BOOST_USE_UCONTEXT)
endif ()
# if (SANITIZERS_ENABLED AND is_clang)
#     # TODO: gcc does not support -fsanitize-blacklist...can we do something else for gcc ?
#     if (NOT Boost_INCLUDE_DIRS AND TARGET Boost::headers)
#         get_target_property(Boost_INCLUDE_DIRS Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
#     endif ()
#     message(STATUS "Adding [${Boost_INCLUDE_DIRS}] to sanitizer blacklist")
#     file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/san_bl.txt "src:${Boost_INCLUDE_DIRS}/*")
#     target_compile_options(opts INTERFACE # ignore boost headers for sanitizing
#                                           -fsanitize-blacklist=${CMAKE_CURRENT_BINARY_DIR}/san_bl.txt)
# endif ()

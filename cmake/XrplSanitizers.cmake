#[===================================================================[
   Configure sanitizers based on environment variables.

   This module reads the following environment variables:
   - SANITIZER: The sanitizers to enable. Possible values:
     - "Address"
     - "Address,UndefinedBehavior"
     - "Thread"
     - "Thread,UndefinedBehavior"

   The compiler type and platform are detected automatically by CMake.
   The sanitizer compile options are applied to the 'common' interface library
   which is linked to all targets in the project.
#]===================================================================]

# Read environment variable
set(SANITIZER $ENV{SANITIZER})

if(SANITIZER)
    message(STATUS "Configuring sanitizers: ${SANITIZER}")

    # Parse SANITIZER value to determine which sanitizers to enable
    set(ENABLE_ASAN FALSE)
    set(ENABLE_TSAN FALSE)
    set(ENABLE_UBSAN FALSE)

    if(SANITIZER MATCHES "Address")
        set(ENABLE_ASAN TRUE)
    endif()
    if(SANITIZER MATCHES "Thread")
        set(ENABLE_TSAN TRUE)
    endif()
    if(SANITIZER MATCHES "UndefinedBehavior")
        set(ENABLE_UBSAN TRUE)
    endif()

    # Detect compiler type
    set(IS_GCC FALSE)
    set(IS_CLANG FALSE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(IS_GCC TRUE)
        message(STATUS "  Compiler: GCC ${CMAKE_CXX_COMPILER_VERSION}")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(IS_CLANG TRUE)
        message(STATUS "  Compiler: Clang ${CMAKE_CXX_COMPILER_VERSION}")
    endif()

    # Detect platform (amd64/x86_64 vs arm64/aarch64)
    set(IS_AMD64 FALSE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(IS_AMD64 TRUE)
        message(STATUS "  Platform: amd64")
    else()
        message(STATUS "  Platform: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    # Frame pointer is required for meaningful stack traces
    set(SANITIZER_COMPILE_FLAGS "-fno-omit-frame-pointer")

    # Sanitizers recommend minimum of -O1 for reasonable performance
    set(SANITIZER_COMPILE_FLAGS "${SANITIZER_COMPILE_FLAGS} -O1")

    # Build the sanitizer flags string
    set(SANITIZER_FLAGS "")

    if(ENABLE_ASAN)
    set(SANITIZER_FLAGS "address")
    elseif(ENABLE_TSAN)
    set(SANITIZER_FLAGS "thread")
    endif()

    if(ENABLE_UBSAN)
        # UB sanitizer flags
        if(IS_CLANG)
            # Clang supports additional UB checks
            set(UBSAN_FLAGS "undefined,float-divide-by-zero,unsigned-integer-overflow")
        else()
            set(UBSAN_FLAGS "undefined,float-divide-by-zero")
        endif()

        if(SANITIZER_FLAGS)
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS},${UBSAN_FLAGS}")
        else()
            set(SANITIZER_FLAGS "${UBSAN_FLAGS}")
        endif()
    endif()

    # Configure code model for GCC on amd64
    # Use large code model for ASAN to avoid relocation errors
    # Use medium code model for TSAN (large is not compatible with TSAN)
    set(SANITIZER_RELOCATION_FLAGS "")
    if(IS_GCC AND IS_AMD64)
        if(ENABLE_ASAN)
            message(STATUS "  Using large code model (-mcmodel=large)")
            set(SANITIZER_COMPILE_FLAGS "${SANITIZER_COMPILE_FLAGS} -mcmodel=large")
            set(SANITIZER_RELOCATION_FLAGS "-mcmodel=large")
        elseif(ENABLE_TSAN)
            message(STATUS "  Using medium code model (-mcmodel=medium)")
            set(SANITIZER_COMPILE_FLAGS "${SANITIZER_COMPILE_FLAGS} -mcmodel=medium")
            set(SANITIZER_RELOCATION_FLAGS "-mcmodel=medium")
        endif()
    endif()

    # Compiler-specific configuration
    if(IS_GCC)
        # Disable mold, gold and lld linkers for GCC with sanitizers
        # Use default linker (bfd/ld) which is more lenient with mixed code models
        set(use_mold OFF CACHE BOOL "Use mold linker" FORCE)
        set(use_gold OFF CACHE BOOL "Use gold linker" FORCE)
        set(use_lld OFF CACHE BOOL "Use lld linker" FORCE)
        message(STATUS "  Disabled mold, gold, and lld linkers for GCC with sanitizers")

        # Suppress false positive warnings in GCC with stringop-overflow
        set(SANITIZER_COMPILE_FLAGS "${SANITIZER_COMPILE_FLAGS} -Wno-stringop-overflow")

        if(ENABLE_TSAN)
            # GCC doesn't support atomic_thread_fence with tsan. Suppress warnings.
            set(SANITIZER_COMPILE_FLAGS "${SANITIZER_COMPILE_FLAGS} -Wno-tsan")
        endif()

        # Add sanitizer to compile and link flags
        set(SANITIZER_COMPILE_FLAGS "${SANITIZER_COMPILE_FLAGS} -fsanitize=${SANITIZER_FLAGS}")
        set(SANITIZER_LINK_FLAGS "${SANITIZER_RELOCATION_FLAGS} -fsanitize=${SANITIZER_FLAGS}")

    elseif(IS_CLANG)
        # Add ignorelist for Clang (GCC doesn't support this)
        # Use CMAKE_SOURCE_DIR to get the path to the ignorelist
        set(IGNORELIST_PATH "${CMAKE_SOURCE_DIR}/sanitizers/suppressions/sanitizer-ignorelist.txt")
        if(EXISTS "${IGNORELIST_PATH}")
            set(SANITIZER_COMPILE_FLAGS "${SANITIZER_COMPILE_FLAGS} -fsanitize-ignorelist=${IGNORELIST_PATH}")
            message(STATUS "  Using sanitizer ignorelist: ${IGNORELIST_PATH}")
        else()
            message(WARNING "Sanitizer ignorelist not found: ${IGNORELIST_PATH}")
        endif()

        # Add sanitizer to compile and link flags
        set(SANITIZER_COMPILE_FLAGS "${SANITIZER_COMPILE_FLAGS} -fsanitize=${SANITIZER_FLAGS}")
        set(SANITIZER_LINK_FLAGS "-fsanitize=${SANITIZER_FLAGS}")
    endif()

    message(STATUS "  Compile flags: ${SANITIZER_COMPILE_FLAGS}")
    message(STATUS "  Link flags: ${SANITIZER_LINK_FLAGS}")

    # Convert space-separated strings to lists for CMake
    separate_arguments(SANITIZER_COMPILE_FLAGS_LIST NATIVE_COMMAND "${SANITIZER_COMPILE_FLAGS}")
    separate_arguments(SANITIZER_LINK_FLAGS_LIST NATIVE_COMMAND "${SANITIZER_LINK_FLAGS}")

    # Apply the sanitizer flags to the 'common' interface library
    # This is the same library used by XrplCompiler.cmake
    target_compile_options(common INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:${SANITIZER_COMPILE_FLAGS_LIST}>
        $<$<COMPILE_LANGUAGE:C>:${SANITIZER_COMPILE_FLAGS_LIST}>
    )

    # Apply linker flags
    target_link_options(common INTERFACE ${SANITIZER_LINK_FLAGS_LIST})

endif()

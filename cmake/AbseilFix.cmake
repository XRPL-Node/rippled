# This file is included by Conan's toolchain file.
# We set the required CMake variable as a CACHE variable to ensure
# it takes effect during the configure step.

set(ABSL_ENABLE_CONSTANT_INIT_V2 "OFF" CACHE BOOL "Disable Abseil's V2 constant init logic to fix compiler errors.")

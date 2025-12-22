from helpers.defs import *
from helpers.enums import *


def generate_config_name(
    os_name: str,
    os_version: str | None,
    compiler_name: str | None,
    compiler_version: str | None,
    arch: Arch,
    build_type: BuildType,
    build_mode: BuildMode,
    build_option: BuildOption,
) -> str:
    """Create a configuration name based on the distro details and build
    attributes.

    The configuration name is used as the display name in the GitHub Actions
    UI, and since GitHub truncates long names we have to make sure the most
    important information is at the beginning of the name.

    Args:
        os_name (str): The OS name.
        os_version (str): The OS version.
        compiler_name (str): The compiler name.
        compiler_version (str): The compiler version.
        arch (Arch): The architecture.
        build_type (BuildType): The build type.
        build_mode (BuildMode): The build mode.
        build_option (BuildOption): The build option.

    Returns:
        str: The configuration name.

    Raises:
        ValueError: If the OS name is empty.
    """

    if not os_name:
        raise ValueError("os_name cannot be empty")

    config_name = os_name
    if os_version:
        config_name += f"-{os_version}"
    if compiler_name:
        config_name += f"-{compiler_name}"
    if compiler_version:
        config_name += f"-{compiler_version}"

    if build_option == BuildOption.COVERAGE:
        config_name += "-coverage"
    elif build_option == BuildOption.VOIDSTAR:
        config_name += "-voidstar"
    elif build_option == BuildOption.SANITIZE_ASAN:
        config_name += "-asan"
    elif build_option == BuildOption.SANITIZE_TSAN:
        config_name += "-tsan"

    if build_type == BuildType.DEBUG:
        config_name += "-debug"
    elif build_type == BuildType.RELEASE:
        config_name += "-release"
    elif build_type == BuildType.PUBLISH:
        config_name += "-publish"

    if build_mode == BuildMode.UNITY_ON:
        config_name += "-unity"

    config_name += f"-{arch.value.split('/')[1]}"

    return config_name


def generate_cmake_args(
    compiler_name: str | None,
    compiler_version: str | None,
    build_type: BuildType,
    build_mode: BuildMode,
    build_option: BuildOption,
    test_option: TestOption,
) -> str:
    """Create the CMake arguments based on the build type and enabled build
    options.

    - All builds will have the `tests`, `werr`, and `xrpld` options.
    - All builds will have the `wextra` option except for GCC 12 and Clang 16.
    - All release builds will have the `assert` option.
    - Set the unity option if specified.
    - Set the coverage option if specified.
    - Set the voidstar option if specified.
    - Set the reference fee if specified.

    Args:
        compiler_name (str): The compiler name.
        compiler_version (str): The compiler version.
        build_type (BuildType): The build type.
        build_mode (BuildMode): The build mode.
        build_option (BuildOption): The build option.
        test_option (TestOption): The test option.

    Returns:
        str: The CMake arguments.

    """

    cmake_args = "-Dtests=ON -Dwerr=ON -Dxrpld=ON"
    if not f"{compiler_name}-{compiler_version}" in [
        "gcc-12",
        "clang-16",
    ]:
        cmake_args += " -Dwextra=ON"

    if build_type == BuildType.RELEASE:
        cmake_args += " -Dassert=ON"

    if build_mode == BuildMode.UNITY_ON:
        cmake_args += " -Dunity=ON"

    if build_option == BuildOption.COVERAGE:
        cmake_args += " -Dcoverage=ON -Dcoverage_format=xml -DCODE_COVERAGE_VERBOSE=ON -DCMAKE_C_FLAGS=-O0 -DCMAKE_CXX_FLAGS=-O0"
    elif build_option == BuildOption.SANITIZE_ASAN:
        pass  # TODO: Add ASAN-UBSAN flags.
    elif build_option == BuildOption.SANITIZE_TSAN:
        pass  # TODO: Add TSAN-UBSAN flags.
    elif build_option == BuildOption.VOIDSTAR:
        cmake_args += " -Dvoidstar=ON"

    if test_option != TestOption.NONE:
        cmake_args += f" -DUNIT_TEST_REFERENCE_FEE={test_option.value}"

    return cmake_args


def generate_cmake_target(os_name: str, build_type: BuildType) -> str:
    """Create the CMake target based on the build type.

    The `install` target is used for Windows and for publishing a package, while
    the `all` target is used for all other configurations.

    Args:
        os_name (str): The OS name.
        build_type (BuildType): The build type.

    Returns:
        str: The CMake target.
    """
    if os_name == "windows" or build_type == BuildType.PUBLISH:
        return "install"
    return "all"


def generate_enable_options(
    os_name: str,
    build_type: BuildType,
    publish_option: PublishOption,
) -> tuple[bool, bool, bool]:
    """Create the enable flags based on the OS name, build option, and publish
     option.

     We build and test all configurations by default, except for Windows in
     Debug, because it is too slow.

    Args:
        os_name (str): The OS name.
        build_type (BuildType): The build type.
        publish_option (PublishOption): The publish option.

    Returns:
        tuple: A tuple containing the enable test, enable package, and enable image flags.
    """
    enable_tests = (
        False if os_name == "windows" and build_type == BuildType.DEBUG else True
    )

    enable_package = (
        True
        if publish_option
        in [
            PublishOption.PACKAGE_ONLY,
            PublishOption.PACKAGE_AND_IMAGE,
        ]
        else False
    )

    enable_image = (
        True
        if publish_option
        in [
            PublishOption.IMAGE_ONLY,
            PublishOption.PACKAGE_AND_IMAGE,
        ]
        else False
    )

    return enable_tests, enable_package, enable_image


def generate_image_name(
    os_name: str,
    os_version: str,
    compiler_name: str,
    compiler_version: str,
    image_sha: str,
) -> str | None:
    """Create the Docker image name based on the distro details.

    Args:
        os_name (str): The OS name.
        os_version (str): The OS version.
        compiler_name (str): The compiler name.
        compiler_version (str): The compiler version.
        image_sha (str): The image SHA.

    Returns:
        str: The Docker image name or None if not applicable.

    Raises:
        ValueError: If any of the arguments is empty for Linux.
    """

    if os_name == "windows" or os_name == "macos":
        return None

    if not os_name:
        raise ValueError("os_name cannot be empty")
    if not os_version:
        raise ValueError("os_version cannot be empty")
    if not compiler_name:
        raise ValueError("compiler_name cannot be empty")
    if not compiler_version:
        raise ValueError("compiler_version cannot be empty")
    if not image_sha:
        raise ValueError("image_sha cannot be empty")

    return f"ghcr.io/xrplf/ci/{os_name}-{os_version}:{compiler_name}-{compiler_version}-{image_sha}"

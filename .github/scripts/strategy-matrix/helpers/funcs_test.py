import pytest

from helpers.enums import *
from helpers.funcs import *


def test_generate_config_name_a_b_c_d_debug_amd64():
    assert (
        generate_config_name(
            "a",
            "b",
            "c",
            "d",
            Arch.LINUX_AMD64,
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
        )
        == "a-b-c-d-debug-amd64"
    )


def test_generate_config_name_a_b_c_release_unity_arm64():
    assert (
        generate_config_name(
            "a",
            "b",
            "c",
            "",
            Arch.LINUX_ARM64,
            BuildType.RELEASE,
            BuildMode.UNITY_ON,
            BuildOption.NONE,
        )
        == "a-b-c-release-unity-arm64"
    )


def test_generate_config_name_a_b_coverage_publish_amd64():
    assert (
        generate_config_name(
            "a",
            "b",
            "",
            "",
            Arch.LINUX_AMD64,
            BuildType.PUBLISH,
            BuildMode.UNITY_OFF,
            BuildOption.COVERAGE,
        )
        == "a-b-coverage-publish-amd64"
    )


def test_generate_config_name_a_asan_debug_unity_arm64():
    assert (
        generate_config_name(
            "a",
            "",
            "",
            "",
            Arch.LINUX_ARM64,
            BuildType.DEBUG,
            BuildMode.UNITY_ON,
            BuildOption.SANITIZE_ASAN,
        )
        == "a-asan-debug-unity-arm64"
    )


def test_generate_config_name_a_c_tsan_release_amd64():
    assert (
        generate_config_name(
            "a",
            "",
            "c",
            "",
            Arch.LINUX_AMD64,
            BuildType.RELEASE,
            BuildMode.UNITY_OFF,
            BuildOption.SANITIZE_TSAN,
        )
        == "a-c-tsan-release-amd64"
    )


def test_generate_config_name_a_d_voidstar_debug_amd64():
    assert (
        generate_config_name(
            "a",
            "",
            "",
            "d",
            Arch.LINUX_AMD64,
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.VOIDSTAR,
        )
        == "a-d-voidstar-debug-amd64"
    )


def test_generate_config_name_raises_on_none_os_name():
    with pytest.raises(ValueError):
        generate_config_name(
            None,
            "b",
            "c",
            "d",
            Arch.LINUX_AMD64,
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
        )


def test_generate_config_name_raises_on_empty_os_name():
    with pytest.raises(ValueError):
        generate_config_name(
            "",
            "b",
            "c",
            "d",
            Arch.LINUX_AMD64,
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
        )


def test_generate_cmake_args_a_b_debug():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
            TestOption.NONE,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON"
    )


def test_generate_cmake_args_gcc_12_no_wextra():
    assert (
        generate_cmake_args(
            "gcc",
            "12",
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
            TestOption.NONE,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON"
    )


def test_generate_cmake_args_clang_16_no_wextra():
    assert (
        generate_cmake_args(
            "clang",
            "16",
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
            TestOption.NONE,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON"
    )


def test_generate_cmake_args_a_b_release():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.RELEASE,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
            TestOption.NONE,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dassert=ON"
    )


def test_generate_cmake_args_a_b_publish():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.PUBLISH,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
            TestOption.NONE,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON"
    )


def test_generate_cmake_args_a_b_unity():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.DEBUG,
            BuildMode.UNITY_ON,
            BuildOption.NONE,
            TestOption.NONE,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dunity=ON"
    )


def test_generate_cmake_args_a_b_coverage():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.COVERAGE,
            TestOption.NONE,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dcoverage=ON -Dcoverage_format=xml -DCODE_COVERAGE_VERBOSE=ON -DCMAKE_C_FLAGS=-O0 -DCMAKE_CXX_FLAGS=-O0"
    )


def test_generate_cmake_args_a_b_voidstar():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.VOIDSTAR,
            TestOption.NONE,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dvoidstar=ON"
    )


def test_generate_cmake_args_a_b_reference_fee_500():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
            TestOption.REFERENCE_FEE_500,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -DUNIT_TEST_REFERENCE_FEE=500"
    )


def test_generate_cmake_args_a_b_reference_fee_1000():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.DEBUG,
            BuildMode.UNITY_OFF,
            BuildOption.NONE,
            TestOption.REFERENCE_FEE_1000,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -DUNIT_TEST_REFERENCE_FEE=1000"
    )


def test_generate_cmake_args_a_b_multiple():
    assert (
        generate_cmake_args(
            "a",
            "b",
            BuildType.RELEASE,
            BuildMode.UNITY_ON,
            BuildOption.VOIDSTAR,
            TestOption.REFERENCE_FEE_500,
        )
        == "-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dassert=ON -Dunity=ON -Dvoidstar=ON -DUNIT_TEST_REFERENCE_FEE=500"
    )


def test_generate_cmake_target_linux_debug():
    assert generate_cmake_target("linux", BuildType.DEBUG) == "all"


def test_generate_cmake_target_linux_release():
    assert generate_cmake_target("linux", BuildType.RELEASE) == "all"


def test_generate_cmake_target_linux_publish():
    assert generate_cmake_target("linux", BuildType.PUBLISH) == "install"


def test_generate_cmake_target_macos_debug():
    assert generate_cmake_target("macos", BuildType.DEBUG) == "all"


def test_generate_cmake_target_macos_release():
    assert generate_cmake_target("macos", BuildType.RELEASE) == "all"


def test_generate_cmake_target_macos_publish():
    assert generate_cmake_target("macos", BuildType.PUBLISH) == "install"


def test_generate_cmake_target_windows_debug():
    assert generate_cmake_target("windows", BuildType.DEBUG) == "install"


def test_generate_cmake_target_windows_release():
    assert generate_cmake_target("windows", BuildType.DEBUG) == "install"


def test_generate_cmake_target_windows_publish():
    assert generate_cmake_target("windows", BuildType.DEBUG) == "install"


def test_generate_enable_options_linux_debug_no_publish():
    assert generate_enable_options("linux", BuildType.DEBUG, PublishOption.NONE) == (
        True,
        False,
        False,
    )


def test_generate_enable_options_linux_release_package_only():
    assert generate_enable_options(
        "linux", BuildType.RELEASE, PublishOption.PACKAGE_ONLY
    ) == (True, True, False)


def test_generate_enable_options_linux_publish_image_only():
    assert generate_enable_options(
        "linux", BuildType.PUBLISH, PublishOption.IMAGE_ONLY
    ) == (True, False, True)


def test_generate_enable_options_macos_debug_package_only():
    assert generate_enable_options(
        "macos", BuildType.DEBUG, PublishOption.PACKAGE_ONLY
    ) == (True, True, False)


def test_generate_enable_options_macos_release_image_only():
    assert generate_enable_options(
        "macos", BuildType.RELEASE, PublishOption.IMAGE_ONLY
    ) == (True, False, True)


def test_generate_enable_options_macos_publish_package_and_image():
    assert generate_enable_options(
        "macos", BuildType.PUBLISH, PublishOption.PACKAGE_AND_IMAGE
    ) == (True, True, True)


def test_generate_enable_options_windows_debug_package_and_image():
    assert generate_enable_options(
        "windows", BuildType.DEBUG, PublishOption.PACKAGE_AND_IMAGE
    ) == (False, True, True)


def test_generate_enable_options_windows_release_no_publish():
    assert generate_enable_options(
        "windows", BuildType.RELEASE, PublishOption.NONE
    ) == (True, False, False)


def test_generate_enable_options_windows_publish_image_only():
    assert generate_enable_options(
        "windows", BuildType.PUBLISH, PublishOption.IMAGE_ONLY
    ) == (True, False, True)


def test_generate_image_name_linux():
    assert generate_image_name("a", "b", "c", "d", "e") == "ghcr.io/xrplf/ci/a-b:c-d-e"


def test_generate_image_name_linux_raises_on_empty_os_name():
    with pytest.raises(ValueError):
        generate_image_name("", "b", "c", "d", "e")


def test_generate_image_name_linux_raises_on_empty_os_version():
    with pytest.raises(ValueError):
        generate_image_name("a", "", "c", "d", "e")


def test_generate_image_name_linux_raises_on_empty_compiler_name():
    with pytest.raises(ValueError):
        generate_image_name("a", "b", "", "d", "e")


def test_generate_image_name_linux_raises_on_empty_compiler_version():
    with pytest.raises(ValueError):
        generate_image_name("a", "b", "c", "", "e")


def test_generate_image_name_linux_raises_on_empty_image_sha():
    with pytest.raises(ValueError):
        generate_image_name("a", "b", "c", "e", "")


def test_generate_image_name_macos():
    assert generate_image_name("macos", "", "", "", "") is None


def test_generate_image_name_macos_extra():
    assert generate_image_name("macos", "value", "does", "not", "matter") is None


def test_generate_image_name_windows():
    assert generate_image_name("windows", "", "", "", "") is None


def test_generate_image_name_windows_extra():
    assert generate_image_name("windows", "value", "does", "not", "matter") is None

import pytest

from generate import *


@pytest.fixture
def macos_distro():
    return Distro(
        os_name="macos",
        specs=[
            Spec(
                archs=[Arch.MACOS_ARM64],
                build_modes=[BuildMode.UNITY_OFF],
                build_option=BuildOption.COVERAGE,
                build_types=[BuildType.RELEASE],
                publish_option=PublishOption.NONE,
                test_option=TestOption.NONE,
                triggers=[Trigger.COMMIT],
            )
        ],
    )


@pytest.fixture
def windows_distro():
    return Distro(
        os_name="windows",
        specs=[
            Spec(
                archs=[Arch.WINDOWS_AMD64],
                build_modes=[BuildMode.UNITY_ON],
                build_option=BuildOption.SANITIZE_ASAN,
                build_types=[BuildType.DEBUG],
                publish_option=PublishOption.IMAGE_ONLY,
                test_option=TestOption.REFERENCE_FEE_500,
                triggers=[Trigger.COMMIT, Trigger.SCHEDULE],
            )
        ],
    )


@pytest.fixture
def linux_distro():
    return Distro(
        os_name="debian",
        os_version="bookworm",
        compiler_name="clang",
        compiler_version="16",
        image_sha="a1b2c3d4",
        specs=[
            Spec(
                archs=[Arch.LINUX_AMD64],
                build_modes=[BuildMode.UNITY_OFF],
                build_option=BuildOption.SANITIZE_TSAN,
                build_types=[BuildType.DEBUG],
                publish_option=PublishOption.NONE,
                test_option=TestOption.NONE,
                triggers=[Trigger.LABEL],
            ),
            Spec(
                archs=[Arch.LINUX_AMD64, Arch.LINUX_ARM64],
                build_modes=[BuildMode.UNITY_OFF, BuildMode.UNITY_ON],
                build_option=BuildOption.VOIDSTAR,
                build_types=[BuildType.PUBLISH],
                publish_option=PublishOption.PACKAGE_AND_IMAGE,
                test_option=TestOption.NONE,
                triggers=[Trigger.COMMIT, Trigger.LABEL],
            ),
        ],
    )


def test_macos_generate_config_for_distro_spec_matches_trigger(macos_distro):
    trigger = Trigger.COMMIT

    distro = macos_distro
    result = list(
        generate_config_for_distro_spec(
            distro.os_name,
            distro.os_version,
            distro.compiler_name,
            distro.compiler_version,
            distro.image_sha,
            distro.specs[0],
            trigger,
        )
    )
    assert result == [
        Config(
            config_name="macos-coverage-release-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dassert=ON -Dcoverage=ON -Dcoverage_format=xml -DCODE_COVERAGE_VERBOSE=ON -DCMAKE_C_FLAGS=-O0 -DCMAKE_CXX_FLAGS=-O0",
            cmake_target="all",
            build_type="Release",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["self-hosted", "macOS", "ARM64", "mac-runner-m1"],
            image=None,
        )
    ]


def test_macos_generate_config_for_distro_spec_no_match_trigger(macos_distro):
    trigger = Trigger.MERGE

    distro = macos_distro
    result = list(
        generate_config_for_distro_spec(
            distro.os_name,
            distro.os_version,
            distro.compiler_name,
            distro.compiler_version,
            distro.image_sha,
            distro.specs[0],
            trigger,
        )
    )
    assert result == []


def test_macos_generate_config_for_distro_matches_trigger(macos_distro):
    trigger = Trigger.COMMIT

    distro = macos_distro
    result = list(generate_config_for_distro(distro, trigger))
    assert result == [
        Config(
            config_name="macos-coverage-release-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dassert=ON -Dcoverage=ON -Dcoverage_format=xml -DCODE_COVERAGE_VERBOSE=ON -DCMAKE_C_FLAGS=-O0 -DCMAKE_CXX_FLAGS=-O0",
            cmake_target="all",
            build_type="Release",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["self-hosted", "macOS", "ARM64", "mac-runner-m1"],
            image=None,
        )
    ]


def test_macos_generate_config_for_distro_no_match_trigger(macos_distro):
    trigger = Trigger.MERGE

    distro = macos_distro
    result = list(generate_config_for_distro(distro, trigger))
    assert result == []


def test_windows_generate_config_for_distro_spec_matches_trigger(
    windows_distro,
):
    trigger = Trigger.COMMIT

    distro = windows_distro
    result = list(
        generate_config_for_distro_spec(
            distro.os_name,
            distro.os_version,
            distro.compiler_name,
            distro.compiler_version,
            distro.image_sha,
            distro.specs[0],
            trigger,
        )
    )
    assert result == [
        Config(
            config_name="windows-asan-debug-unity-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dunity=ON -DUNIT_TEST_REFERENCE_FEE=500",
            cmake_target="install",
            build_type="Debug",
            enable_tests=False,
            enable_package=False,
            enable_image=True,
            runs_on=["self-hosted", "Windows", "devbox"],
            image=None,
        )
    ]


def test_windows_generate_config_for_distro_spec_no_match_trigger(
    windows_distro,
):
    trigger = Trigger.MERGE

    distro = windows_distro
    result = list(
        generate_config_for_distro_spec(
            distro.os_name,
            distro.os_version,
            distro.compiler_name,
            distro.compiler_version,
            distro.image_sha,
            distro.specs[0],
            trigger,
        )
    )
    assert result == []


def test_windows_generate_config_for_distro_matches_trigger(
    windows_distro,
):
    trigger = Trigger.COMMIT

    distro = windows_distro
    result = list(generate_config_for_distro(distro, trigger))
    assert result == [
        Config(
            config_name="windows-asan-debug-unity-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dunity=ON -DUNIT_TEST_REFERENCE_FEE=500",
            cmake_target="install",
            build_type="Debug",
            enable_tests=False,
            enable_package=False,
            enable_image=True,
            runs_on=["self-hosted", "Windows", "devbox"],
            image=None,
        )
    ]


def test_windows_generate_config_for_distro_no_match_trigger(
    windows_distro,
):
    trigger = Trigger.MERGE

    distro = windows_distro
    result = list(generate_config_for_distro(distro, trigger))
    assert result == []


def test_linux_generate_config_for_distro_spec_matches_trigger(linux_distro):
    trigger = Trigger.LABEL

    distro = linux_distro
    result = list(
        generate_config_for_distro_spec(
            distro.os_name,
            distro.os_version,
            distro.compiler_name,
            distro.compiler_version,
            distro.image_sha,
            distro.specs[1],
            trigger,
        )
    )
    assert result == [
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "X64", "heavy"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-unity-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dunity=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "X64", "heavy"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "ARM64", "heavy-arm64"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-unity-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dunity=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "ARM64", "heavy-arm64"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
    ]


def test_linux_generate_config_for_distro_spec_no_match_trigger(linux_distro):
    trigger = Trigger.MERGE

    distro = linux_distro
    result = list(
        generate_config_for_distro_spec(
            distro.os_name,
            distro.os_version,
            distro.compiler_name,
            distro.compiler_version,
            distro.image_sha,
            distro.specs[1],
            trigger,
        )
    )
    assert result == []


def test_linux_generate_config_for_distro_matches_trigger(linux_distro):
    trigger = Trigger.LABEL

    distro = linux_distro
    result = list(generate_config_for_distro(distro, trigger))
    assert result == [
        Config(
            config_name="debian-bookworm-clang-16-tsan-debug-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["self-hosted", "Linux", "X64", "heavy"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "X64", "heavy"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-unity-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dunity=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "X64", "heavy"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "ARM64", "heavy-arm64"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-unity-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dunity=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "ARM64", "heavy-arm64"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
    ]


def test_linux_generate_config_for_distro_no_match_trigger(linux_distro):
    trigger = Trigger.MERGE

    distro = linux_distro
    result = list(generate_config_for_distro(distro, trigger))
    assert result == []


def test_generate_configs(macos_distro, windows_distro, linux_distro):
    trigger = Trigger.COMMIT

    distros = [macos_distro, windows_distro, linux_distro]
    result = generate_configs(distros, trigger)
    assert result == [
        Config(
            config_name="macos-coverage-release-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dassert=ON -Dcoverage=ON -Dcoverage_format=xml -DCODE_COVERAGE_VERBOSE=ON -DCMAKE_C_FLAGS=-O0 -DCMAKE_CXX_FLAGS=-O0",
            cmake_target="all",
            build_type="Release",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["self-hosted", "macOS", "ARM64", "mac-runner-m1"],
            image=None,
        ),
        Config(
            config_name="windows-asan-debug-unity-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dwextra=ON -Dunity=ON -DUNIT_TEST_REFERENCE_FEE=500",
            cmake_target="install",
            build_type="Debug",
            enable_tests=False,
            enable_package=False,
            enable_image=True,
            runs_on=["self-hosted", "Windows", "devbox"],
            image=None,
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "X64", "heavy"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-unity-amd64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dunity=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "X64", "heavy"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "ARM64", "heavy-arm64"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
        Config(
            config_name="debian-bookworm-clang-16-voidstar-publish-unity-arm64",
            cmake_args="-Dtests=ON -Dwerr=ON -Dxrpld=ON -Dunity=ON -Dvoidstar=ON",
            cmake_target="install",
            build_type="Release",
            enable_tests=True,
            enable_package=True,
            enable_image=True,
            runs_on=["self-hosted", "Linux", "ARM64", "heavy-arm64"],
            image="ghcr.io/xrplf/ci/debian-bookworm:clang-16-a1b2c3d4",
        ),
    ]


def test_generate_configs_raises_on_duplicate_configs(
    macos_distro, windows_distro, linux_distro
):
    trigger = Trigger.COMMIT

    distros = [macos_distro, macos_distro]
    with pytest.raises(ValueError):
        generate_configs(distros, trigger)

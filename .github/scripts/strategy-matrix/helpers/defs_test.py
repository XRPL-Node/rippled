import pytest

from helpers.defs import *
from helpers.enums import *
from helpers.funcs import *


def test_config_valid_none_image():
    assert Config(
        config_name="config",
        cmake_args="-Doption=ON",
        cmake_target="all",
        build_type="Debug",
        enable_tests=True,
        enable_package=False,
        enable_image=False,
        runs_on=["label"],
        image=None,
    )


def test_config_valid_empty_image():
    assert Config(
        config_name="config",
        cmake_args="-Doption=ON",
        cmake_target="install",
        build_type="Debug",
        enable_tests=False,
        enable_package=True,
        enable_image=False,
        runs_on=["label"],
        image="",
    )


def test_config_valid_with_image():
    assert Config(
        config_name="config",
        cmake_args="-Doption=ON",
        cmake_target="install",
        build_type="Release",
        enable_tests=False,
        enable_package=True,
        enable_image=True,
        runs_on=["label"],
        image="image",
    )


def test_config_raises_on_empty_config_name():
    with pytest.raises(ValueError):
        Config(
            config_name="",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_wrong_config_name():
    with pytest.raises(TypeError):
        Config(
            config_name=123,
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_empty_cmake_args():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_wrong_cmake_args():
    with pytest.raises(TypeError):
        Config(
            config_name="config",
            cmake_args=123,
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_empty_cmake_target():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_invalid_cmake_target():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="invalid",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_wrong_cmake_target():
    with pytest.raises(TypeError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target=123,
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_empty_build_type():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_invalid_build_type():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="invalid",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_wrong_build_type():
    with pytest.raises(TypeError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type=123,
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_wrong_enable_tests():
    with pytest.raises(TypeError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=123,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_wrong_enable_package():
    with pytest.raises(TypeError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=123,
            enable_image=False,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_wrong_enable_image():
    with pytest.raises(TypeError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=True,
            enable_image=123,
            runs_on=["label"],
            image="image",
        )


def test_config_raises_on_none_runs_on():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=None,
            image="image",
        )


def test_config_raises_on_empty_runs_on():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=[],
            image="image",
        )


def test_config_raises_on_invalid_runs_on():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=[""],
            image="image",
        )


def test_config_raises_on_wrong_runs_on():
    with pytest.raises(TypeError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=[123],
            image="image",
        )


def test_config_raises_on_duplicate_runs_on():
    with pytest.raises(ValueError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label", "label"],
            image="image",
        )


def test_config_raises_on_wrong_image():
    with pytest.raises(TypeError):
        Config(
            config_name="config",
            cmake_args="-Doption=ON",
            cmake_target="all",
            build_type="Debug",
            enable_tests=True,
            enable_package=False,
            enable_image=False,
            runs_on=["label"],
            image=123,
        )


def test_spec_valid():
    assert Spec(
        archs=[Arch.LINUX_AMD64],
        build_option=BuildOption.NONE,
        build_modes=[BuildMode.UNITY_OFF],
        build_types=[BuildType.DEBUG],
        publish_option=PublishOption.NONE,
        test_option=TestOption.NONE,
        triggers=[Trigger.COMMIT],
    )


def test_spec_raises_on_none_archs():
    with pytest.raises(ValueError):
        Spec(
            archs=None,
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_empty_archs():
    with pytest.raises(ValueError):
        Spec(
            archs=[],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_wrong_archs():
    with pytest.raises(TypeError):
        Spec(
            archs=[123],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_duplicate_archs():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64, Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_wrong_build_option():
    with pytest.raises(TypeError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=123,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_none_build_modes():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=None,
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_empty_build_modes():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_wrong_build_modes():
    with pytest.raises(TypeError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[123],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_none_build_types():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=None,
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_empty_build_types():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_wrong_build_types():
    with pytest.raises(TypeError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[123],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_duplicate_build_types():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG, BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_wrong_publish_option():
    with pytest.raises(TypeError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=123,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_wrong_test_option():
    with pytest.raises(TypeError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=123,
            triggers=[Trigger.COMMIT],
        )


def test_spec_raises_on_none_triggers():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=None,
        )


def test_spec_raises_on_empty_triggers():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[],
        )


def test_spec_raises_on_wrong_triggers():
    with pytest.raises(TypeError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[123],
        )


def test_spec_raises_on_duplicate_triggers():
    with pytest.raises(ValueError):
        Spec(
            archs=[Arch.LINUX_AMD64],
            build_option=BuildOption.NONE,
            build_modes=[BuildMode.UNITY_OFF],
            build_types=[BuildType.DEBUG],
            publish_option=PublishOption.NONE,
            test_option=TestOption.NONE,
            triggers=[Trigger.COMMIT, Trigger.COMMIT],
        )


def test_distro_valid_none_image_sha():
    assert Distro(
        os_name="os_name",
        os_version="os_version",
        compiler_name="compiler_name",
        compiler_version="compiler_version",
        image_sha=None,
        specs=[Spec()],  # This is valid due to the default values.
    )


def test_distro_valid_empty_os_compiler_image_sha():
    assert Distro(
        os_name="os_name",
        os_version="",
        compiler_name="",
        compiler_version="",
        image_sha="",
        specs=[Spec()],
    )


def test_distro_valid_with_image():
    assert Distro(
        os_name="os_name",
        os_version="os_version",
        compiler_name="compiler_name",
        compiler_version="compiler_version",
        image_sha="image_sha",
        specs=[Spec()],
    )


def test_distro_raises_on_empty_os_name():
    with pytest.raises(ValueError):
        Distro(
            os_name="",
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=[Spec()],
        )


def test_distro_raises_on_wrong_os_name():
    with pytest.raises(TypeError):
        Distro(
            os_name=123,
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=[Spec()],
        )


def test_distro_raises_on_wrong_os_version():
    with pytest.raises(TypeError):
        Distro(
            os_name="os_name",
            os_version=123,
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=[Spec()],
        )


def test_distro_raises_on_wrong_compiler_name():
    with pytest.raises(TypeError):
        Distro(
            os_name="os_name",
            os_version="os_version",
            compiler_name=123,
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=[Spec()],
        )


def test_distro_raises_on_wrong_compiler_version():
    with pytest.raises(TypeError):
        Distro(
            os_name="os_name",
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version=123,
            image_sha="image_sha",
            specs=[Spec()],
        )


def test_distro_raises_on_wrong_image_sha():
    with pytest.raises(TypeError):
        Distro(
            os_name="os_name",
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha=123,
            specs=[Spec()],
        )


def test_distro_raises_on_none_specs():
    with pytest.raises(ValueError):
        Distro(
            os_name="os_name",
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=None,
        )


def test_distro_raises_on_empty_specs():
    with pytest.raises(ValueError):
        Distro(
            os_name="os_name",
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=[],
        )


def test_distro_raises_on_invalid_specs():
    with pytest.raises(ValueError):
        Distro(
            os_name="os_name",
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=[Spec(triggers=[])],
        )


def test_distro_raises_on_duplicate_specs():
    with pytest.raises(ValueError):
        Distro(
            os_name="os_name",
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=[Spec(), Spec()],
        )


def test_distro_raises_on_wrong_specs():
    with pytest.raises(TypeError):
        Distro(
            os_name="os_name",
            os_version="os_version",
            compiler_name="compiler_name",
            compiler_version="compiler_version",
            image_sha="image_sha",
            specs=[123],
        )

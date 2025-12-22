from dataclasses import dataclass, field

from helpers.enums import *
from helpers.unique import *


@dataclass
class Config:
    """Represents a configuration to include in the strategy matrix.

    Raises:
        ValueError: If any of the required fields are empty or invalid.
        TypeError: If any of the required fields are of the wrong type.
    """

    config_name: str
    cmake_args: str
    cmake_target: str
    build_type: str
    enable_tests: bool
    enable_package: bool
    enable_image: bool
    runs_on: list[str]
    image: str | None = None

    def __post_init__(self):
        if not self.config_name:
            raise ValueError("config_name cannot be empty")
        if not isinstance(self.config_name, str):
            raise TypeError("config_name must be a string")

        if not self.cmake_args:
            raise ValueError("cmake_args cannot be empty")
        if not isinstance(self.cmake_args, str):
            raise TypeError("cmake_args must be a string")

        if not self.cmake_target:
            raise ValueError("cmake_target cannot be empty")
        if not isinstance(self.cmake_target, str):
            raise TypeError("cmake_target must be a string")
        if self.cmake_target not in ["all", "install"]:
            raise ValueError("cmake_target must be 'all' or 'install'")

        if not self.build_type:
            raise ValueError("build_type cannot be empty")
        if not isinstance(self.build_type, str):
            raise TypeError("build_type must be a string")
        if self.build_type not in ["Debug", "Release"]:
            raise ValueError("build_type must be 'Debug' or 'Release'")

        if not isinstance(self.enable_tests, bool):
            raise TypeError("enable_tests must be a boolean")
        if not isinstance(self.enable_package, bool):
            raise TypeError("enable_package must be a boolean")
        if not isinstance(self.enable_image, bool):
            raise TypeError("enable_image must be a boolean")

        if not self.runs_on:
            raise ValueError("runs_on cannot be empty")
        if not isinstance(self.runs_on, list):
            raise TypeError("runs_on must be a list")
        if not all(isinstance(runner, str) for runner in self.runs_on):
            raise TypeError("runs_on must be a list of strings")
        if not all(self.runs_on):
            raise ValueError("runs_on must be a list of non-empty strings")
        if len(self.runs_on) != len(set(self.runs_on)):
            raise ValueError("runs_on must be a list of unique strings")

        if self.image and not isinstance(self.image, str):
            raise TypeError("image must be a string")


@dataclass
class Spec:
    """Represents a specification used by a configuration.

    Raises:
        ValueError: If any of the required fields are empty.
        TypeError: If any of the required fields are of the wrong type.
    """

    archs: list[Arch] = field(
        default_factory=lambda: [Arch.LINUX_AMD64, Arch.LINUX_ARM64]
    )
    build_option: BuildOption = BuildOption.NONE
    build_modes: list[BuildMode] = field(
        default_factory=lambda: [BuildMode.UNITY_OFF, BuildMode.UNITY_ON]
    )
    build_types: list[BuildType] = field(
        default_factory=lambda: [BuildType.DEBUG, BuildType.RELEASE]
    )
    publish_option: PublishOption = PublishOption.NONE
    test_option: TestOption = TestOption.NONE
    triggers: list[Trigger] = field(
        default_factory=lambda: [Trigger.COMMIT, Trigger.MERGE, Trigger.SCHEDULE]
    )

    def __post_init__(self):
        if not self.archs:
            raise ValueError("archs cannot be empty")
        if not isinstance(self.archs, list):
            raise TypeError("archs must be a list")
        if not all(isinstance(arch, str) for arch in self.archs):
            raise TypeError("archs must be a list of Arch")
        if len(self.archs) != len(set(self.archs)):
            raise ValueError("archs must be a list of unique Arch")

        if not isinstance(self.build_option, BuildOption):
            raise TypeError("build_option must be a BuildOption")

        if not self.build_modes:
            raise ValueError("build_modes cannot be empty")
        if not isinstance(self.build_modes, list):
            raise TypeError("build_modes must be a list")
        if not all(
            isinstance(build_mode, BuildMode) for build_mode in self.build_modes
        ):
            raise TypeError("build_modes must be a list of BuildMode")
        if len(self.build_modes) != len(set(self.build_modes)):
            raise ValueError("build_modes must be a list of unique BuildMode")

        if not self.build_types:
            raise ValueError("build_types cannot be empty")
        if not isinstance(self.build_types, list):
            raise TypeError("build_types must be a list")
        if not all(
            isinstance(build_type, BuildType) for build_type in self.build_types
        ):
            raise TypeError("build_types must be a list of BuildType")
        if len(self.build_types) != len(set(self.build_types)):
            raise ValueError("build_types must be a list of unique BuildType")

        if not isinstance(self.publish_option, PublishOption):
            raise TypeError("publish_option must be a PublishOption")

        if not isinstance(self.test_option, TestOption):
            raise TypeError("test_option must be a TestOption")

        if not self.triggers:
            raise ValueError("triggers cannot be empty")
        if not isinstance(self.triggers, list):
            raise TypeError("triggers must be a list")
        if not all(isinstance(trigger, Trigger) for trigger in self.triggers):
            raise TypeError("triggers must be a list of Trigger")
        if len(self.triggers) != len(set(self.triggers)):
            raise ValueError("triggers must be a list of unique Trigger")


@dataclass
class Distro:
    """Represents a Linux, Windows or macOS distribution with specifications.

    Raises:
        ValueError: If any of the required fields are empty.
        TypeError: If any of the required fields are of the wrong type.
    """

    os_name: str
    os_version: str = ""
    compiler_name: str = ""
    compiler_version: str = ""
    image_sha: str = ""
    specs: list[Spec] = field(default_factory=list)

    def __post_init__(self):
        if not self.os_name:
            raise ValueError("os_name cannot be empty")
        if not isinstance(self.os_name, str):
            raise TypeError("os_name must be a string")

        if self.os_version and not isinstance(self.os_version, str):
            raise TypeError("os_version must be a string")

        if self.compiler_name and not isinstance(self.compiler_name, str):
            raise TypeError("compiler_name must be a string")

        if self.compiler_version and not isinstance(self.compiler_version, str):
            raise TypeError("compiler_version must be a string")

        if self.image_sha and not isinstance(self.image_sha, str):
            raise TypeError("image_sha must be a string")

        if not self.specs:
            raise ValueError("specs cannot be empty")
        if not isinstance(self.specs, list):
            raise TypeError("specs must be a list")
        if not all(isinstance(spec, Spec) for spec in self.specs):
            raise TypeError("specs must be a list of Spec")
        if not is_unique(self.specs):
            raise ValueError("specs must be a list of unique Spec")

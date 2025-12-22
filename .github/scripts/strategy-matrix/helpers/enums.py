from enum import StrEnum, auto


class Arch(StrEnum):
    """Represents architectures to build for."""

    LINUX_AMD64 = "linux/amd64"
    LINUX_ARM64 = "linux/arm64"
    MACOS_ARM64 = "macos/arm64"
    WINDOWS_AMD64 = "windows/amd64"


class BuildMode(StrEnum):
    """Represents whether to perform a unity or non-unity build."""

    UNITY_OFF = auto()
    UNITY_ON = auto()


class BuildOption(StrEnum):
    """Represents build options to enable."""

    NONE = auto()
    COVERAGE = auto()
    SANITIZE_ASAN = (
        auto()
    )  # Address Sanitizer, also includes Undefined Behavior Sanitizer.
    SANITIZE_TSAN = (
        auto()
    )  # Thread Sanitizer, also includes Undefined Behavior Sanitizer.
    VOIDSTAR = auto()


class TestOption(StrEnum):
    """Represents test options to enable, specifically the reference fee to use."""

    __test__ = False  # Tell pytest to not consider this as a test class.

    NONE = ""  # Use the default reference fee of 10.
    REFERENCE_FEE_500 = "500"
    REFERENCE_FEE_1000 = "1000"


class PublishOption(StrEnum):
    """Represents whether to publish a package, an image, or both."""

    NONE = auto()
    PACKAGE_ONLY = auto()
    IMAGE_ONLY = auto()
    PACKAGE_AND_IMAGE = auto()


class BuildType(StrEnum):
    """Represents the build type to use."""

    DEBUG = auto()
    RELEASE = auto()
    PUBLISH = auto()  # Release build without assertions.


class Platform(StrEnum):
    """Represents the platform to use."""

    LINUX = "linux"
    MACOS = "macos"
    WINDOWS = "windows"


class Trigger(StrEnum):
    """Represents the trigger that caused the workflow to run."""

    COMMIT = "commit"
    LABEL = "label"
    MERGE = "merge"
    SCHEDULE = "schedule"

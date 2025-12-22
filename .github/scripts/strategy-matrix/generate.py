#!/usr/bin/env python3
import argparse
import dataclasses
import itertools
from collections.abc import Iterator

import linux
import macos
import windows
from helpers.defs import *
from helpers.enums import *
from helpers.funcs import *
from helpers.unique import *

# The GitHub runner tags to use for the different architectures.
RUNNER_TAGS = {
    Arch.LINUX_AMD64: ["self-hosted", "Linux", "X64", "heavy"],
    Arch.LINUX_ARM64: ["self-hosted", "Linux", "ARM64", "heavy-arm64"],
    Arch.MACOS_ARM64: ["self-hosted", "macOS", "ARM64", "mac-runner-m1"],
    Arch.WINDOWS_AMD64: ["self-hosted", "Windows", "devbox"],
}


def generate_configs(distros: list[Distro], trigger: Trigger) -> list[Config]:
    """Generate a strategy matrix for GitHub Actions CI.

    Args:
        distros: The distros to generate the matrix for.
        trigger: The trigger that caused the workflow to run.

    Returns:
        list[Config]: The generated configurations.

    Raises:
        ValueError: If any of the required fields are empty or invalid.
        TypeError: If any of the required fields are of the wrong type.

    """

    configs = []
    for distro in distros:
        for config in generate_config_for_distro(distro, trigger):
            configs.append(config)

    if not is_unique(configs):
        raise ValueError("configs must be a list of unique Config")

    return configs


def generate_config_for_distro(distro: Distro, trigger: Trigger) -> Iterator[Config]:
    """Generate a strategy matrix for a specific distro.

    Args:
        distro: The distro to generate the matrix for.
        trigger: The trigger that caused the workflow to run.

    Yields:
        Config: The next configuration to build.

    Raises:
        ValueError: If any of the required fields are empty or invalid.
        TypeError: If any of the required fields are of the wrong type.

    """
    for spec in distro.specs:
        if trigger not in spec.triggers:
            continue

        os_name = distro.os_name
        os_version = distro.os_version
        compiler_name = distro.compiler_name
        compiler_version = distro.compiler_version
        image_sha = distro.image_sha
        yield from generate_config_for_distro_spec(
            os_name,
            os_version,
            compiler_name,
            compiler_version,
            image_sha,
            spec,
            trigger,
        )


def generate_config_for_distro_spec(
    os_name: str,
    os_version: str,
    compiler_name: str,
    compiler_version: str,
    image_sha: str,
    spec: Spec,
    trigger: Trigger,
) -> Iterator[Config]:
    """Generate a strategy matrix for a specific distro and spec.

    Args:
        os_name: The OS name.
        os_version: The OS version.
        compiler_name: The compiler name.
        compiler_version: The compiler version.
        image_sha: The image SHA.
        spec: The spec to generate the matrix for.
        trigger: The trigger that caused the workflow to run.

    Yields:
        Config: The next configuration to build.

    """

    for trigger_, arch, build_mode, build_type in itertools.product(
        spec.triggers, spec.archs, spec.build_modes, spec.build_types
    ):
        if trigger_ != trigger:
            continue

        build_option = spec.build_option
        test_option = spec.test_option
        publish_option = spec.publish_option

        # Determine the configuration name.
        config_name = generate_config_name(
            os_name,
            os_version,
            compiler_name,
            compiler_version,
            arch,
            build_type,
            build_mode,
            build_option,
        )

        # Determine the CMake arguments.
        cmake_args = generate_cmake_args(
            compiler_name,
            compiler_version,
            build_type,
            build_mode,
            build_option,
            test_option,
        )

        # Determine the CMake target.
        cmake_target = generate_cmake_target(os_name, build_type)

        # Determine whether to enable running tests, and to create a package
        # and/or image.
        enable_tests, enable_package, enable_image = generate_enable_options(
            os_name, build_type, publish_option
        )

        # Determine the image to run in, if applicable.
        image = generate_image_name(
            os_name,
            os_version,
            compiler_name,
            compiler_version,
            image_sha,
        )

        # Generate the configuration.
        yield Config(
            config_name=config_name,
            cmake_args=cmake_args,
            cmake_target=cmake_target,
            build_type=("Debug" if build_type == BuildType.DEBUG else "Release"),
            enable_tests=enable_tests,
            enable_package=enable_package,
            enable_image=enable_image,
            runs_on=RUNNER_TAGS[arch],
            image=image,
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--platform",
        "-p",
        required=False,
        type=Platform,
        choices=list(Platform),
        help="The platform to run on.",
    )
    parser.add_argument(
        "--trigger",
        "-t",
        required=True,
        type=Trigger,
        choices=list(Trigger),
        help="The trigger that caused the workflow to run.",
    )
    args = parser.parse_args()

    # Collect the distros to generate configs for.
    distros = []
    if args.platform in [None, Platform.LINUX]:
        distros += linux.DEBIAN_DISTROS + linux.RHEL_DISTROS + linux.UBUNTU_DISTROS
    if args.platform in [None, Platform.MACOS]:
        distros += macos.DISTROS
    if args.platform in [None, Platform.WINDOWS]:
        distros += windows.DISTROS

    # Generate the configs.
    configs = generate_configs(distros, args.trigger)

    # Convert the configs into the format expected by GitHub Actions.
    include = []
    for config in configs:
        include.append(dataclasses.asdict(config))
    print(f"matrix={json.dumps({'include': include})}")

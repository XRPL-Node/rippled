from helpers.defs import Distro, Spec
from helpers.enums import Arch

DISTROS = [
    Distro(
        os_name="macos",
        specs=[Spec(archs=[Arch.MACOS_ARM64])],
    ),
]

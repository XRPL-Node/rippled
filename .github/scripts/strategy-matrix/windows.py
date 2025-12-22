from helpers.defs import Distro, Spec
from helpers.enums import Arch

DISTROS = [
    Distro(
        os_name="windows",
        specs=[Spec(archs=[Arch.WINDOWS_AMD64])],
    ),
]

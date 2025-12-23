from helpers.defs import *
from helpers.enums import *

DISTROS = [
    Distro(
        os_name="macos",
        specs=[
            Spec(
                archs=[Arch.MACOS_ARM64],
                build_modes=[BuildMode.UNITY_OFF],
                build_types=[BuildType.DEBUG],
                triggers=[Trigger.COMMIT, Trigger.MERGE],
            ),
            Spec(
                archs=[Arch.MACOS_ARM64],
                triggers=[Trigger.SCHEDULE],
            ),
        ],
    ),
]

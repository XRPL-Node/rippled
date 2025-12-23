from helpers.defs import *
from helpers.enums import *

DISTROS = [
    Distro(
        os_name="windows",
        specs=[
            Spec(
                archs=[Arch.WINDOWS_AMD64],
                build_modes=[BuildMode.UNITY_ON],
                build_types=[BuildType.RELEASE],
                triggers=[Trigger.COMMIT, Trigger.MERGE],
            ),
            Spec(
                archs=[Arch.WINDOWS_AMD64],
                triggers=[Trigger.SCHEDULE],
            ),
        ],
    ),
]

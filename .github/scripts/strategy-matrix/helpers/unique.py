from dataclasses import asdict, _is_dataclass_instance
import json
from typing import Any


def is_unique(items: list[Any]) -> bool:
    """Check if a list of dataclass objects contains only unique items.

    As the items may not be hashable, we convert them to json strings first, and
    then check if the list of strings is the same size as the set of strings.

    Args:
        items: The list of dataclass objects to check.

    Returns:
        True if the list contains only unique items, False otherwise.

    Raises:
        TypeError: If any of the items is not a dataclass.
    """

    l = list()
    s = set()
    for item in items:
        if not _is_dataclass_instance(item):
            raise TypeError("items must be a list of dataclasses")
        j = json.dumps(asdict(item))
        l.append(j)
        s.add(j)
    return len(l) == len(s)

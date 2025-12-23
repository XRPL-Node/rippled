from dataclasses import dataclass

import pytest

from helpers.unique import *


@dataclass
class ExampleInt:
    value: int


@dataclass
class ExampleList:
    values: list[int]


def test_unique_int():
    assert is_unique([ExampleInt(1), ExampleInt(2), ExampleInt(3)])


def test_not_unique_int():
    assert not is_unique([ExampleInt(1), ExampleInt(2), ExampleInt(1)])


def test_unique_list():
    assert is_unique(
        [ExampleList([1, 2, 3]), ExampleList([4, 5, 6]), ExampleList([7, 8, 9])]
    )


def test_not_unique_list():
    assert not is_unique(
        [ExampleList([1, 2, 3]), ExampleList([4, 5, 6]), ExampleList([1, 2, 3])]
    )


def test_unique_raises_on_non_dataclass():
    with pytest.raises(TypeError):
        is_unique([1, 2, 3])

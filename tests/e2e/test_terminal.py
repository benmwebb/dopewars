"""Checks of the terminal driver itself (see terminal.py), so that a
broken driver isn't mistaken for a broken curses client."""

import os

import pytest

from conftest import IS_WINDOWS


def test_echo(tmp_path):
    if not IS_WINDOWS:
        pytest.importorskip('pexpect')
    pytest.importorskip('pyte')
    from terminal import Terminal
    if IS_WINDOWS:
        argv = ['cmd.exe', '/c', 'echo hello terminal']
    else:
        argv = ['/bin/echo', 'hello terminal']
    t = Terminal(argv, dict(os.environ), str(tmp_path))
    t.wait_for('hello terminal')
    assert t.wait_exit() == 0

'''
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
'''

import logging
import signal
import subprocess
import sys
import threading
from typing import List, Union

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


def run_in_shell(commands: List[Union[List[str], str]]):
    """
    str commands are not escaped automatically, so you get full control over it. Tokens of List[str]
    commands will be escaped when containing whitespace.

    :return: (return_code, output)
    """
    output = ""

    def shell_reader(process: subprocess.Popen):
        global logger
        nonlocal output

        for line in iter(process.stdout.readline, b""):
            if line.strip():
                output += line
                logger.info(line.rstrip())

            if len(line) == 0:
                break

    shell_name = {"win32": "cmd.exe", "darwin": "zsh", "linux": "bash"}

    process = subprocess.Popen(
        shell_name[sys.platform],
        shell=True,
        universal_newlines=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    def kill_at_exit():
        process.kill()

    signal.signal(signal.SIGTERM, kill_at_exit)
    signal.signal(signal.SIGINT, kill_at_exit)

    t = threading.Thread(target=shell_reader, args=(process,))

    for cmd in commands:
        if isinstance(cmd, list):
            cmd = [token if (" " not in token or '"' in token) else f'"{token}"' for token in cmd]
            cmd = " ".join(cmd)
        logger.debug(f"run_in_shell: executing command: {cmd}")
        process.stdin.write(cmd + "\n")

    if sys.platform == "win32":
        process.stdin.write("exit /b %errorlevel%\n")
    process.stdin.flush()
    process.stdin.close()

    t.start()

    return_code = None

    while return_code is None:
        try:
            return_code = process.wait(0.5)
        except subprocess.TimeoutExpired:
            pass

    t.join()
    assert return_code == 0, output
    return return_code, output

"""
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
"""

import logging
import os
import sys
import shutil
import tempfile
import pathlib

import bbmp_subprocess
import bbmp_util as util


def rel_to_py(*paths):
    return os.path.realpath(os.path.join(os.path.realpath(os.path.dirname(__file__)), *paths))


logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

PROJECT_DIR = rel_to_py("..")

VCVARSALL_COMMAND = []
PLATFORM_SPECIFIC_CMAKE_PARAMETERS = []

SCRIPT_SELF_PATH = pathlib.PurePath(os.path.realpath(__file__)).as_posix()

if sys.platform == "win32":
    vcvarsall = util.get_most_recent_file(
        util.find_file(
            "vcvarsall.bat",
            root="C:\\",
            search_constraint=["Program Files", "Visual Studio"],
            cache_result=False,
        )
    )
    assert vcvarsall, "vcvarsall.bat not found"

    VCVARSALL_COMMAND = [vcvarsall, "x86_amd64"]
    PLATFORM_SPECIFIC_CMAKE_PARAMETERS = ["-G", "NMake Makefiles"]

if __name__ == "__main__":
    logging.basicConfig()


    install_dir = None

    with util.Cache(f"{SCRIPT_SELF_PATH}.cache.txt") as cache:
        INSTALL_DIR_KEY = "install_dir"
        default_install_dir = rel_to_py("..", "install")

        if INSTALL_DIR_KEY not in cache.data:
            print("")
            print(f"Specify target directory for binary artifacts (will be created if necessary):")
            print(f"Just hitting enter with an empty line will default to [{default_install_dir}]")
            cache.data[INSTALL_DIR_KEY] = input("> ").strip() or default_install_dir
            print("")

        install_dir = cache.get_or_else(INSTALL_DIR_KEY, default_install_dir)

        if not os.path.exists(install_dir):
            os.makedirs(install_dir)

        print(f"Building and installing artifacts to {install_dir}")

    with tempfile.TemporaryDirectory() as cmake_build_dir:
        commands = [
            VCVARSALL_COMMAND,
            [
                "MSBuild.exe",
                rel_to_py("..", "temperature_reader", "temperature_reader.sln"),
                "/p:Configuration=Release",
                '/p:Platform="Any CPU"',
                f'/p:OutDir="{install_dir}"',
            ],
            [
                "cmake",
                f"-DCMAKE_INSTALL_PREFIX={install_dir}",
                "-DCMAKE_BUILD_TYPE=Release",
                "-S",
                PROJECT_DIR,
                "-B",
                cmake_build_dir,
                *PLATFORM_SPECIFIC_CMAKE_PARAMETERS,  # selects generator on Windows
            ],
            [
                "cmake",
                "--build",
                cmake_build_dir,
                "--config",
                "Release",
                "--target",
                "install",
            ],
        ]
        bbmp_subprocess.run_in_shell(commands)

    # Prepare files in order the user has InnoSetup and wants to create an installer
    os.rename(
        os.path.join(install_dir, "Bebump Coolth.exe"), 
        os.path.join(install_dir, "coolth.exe"),
    )
    shutil.copy(
        rel_to_py("..", "installer", "coolth.iss"),
        os.path.join(install_dir),
    )
    shutil.copy(
        rel_to_py("..", "installer", "license.txt"),
        os.path.join(install_dir),
    )

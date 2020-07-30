'''
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
'''

import json
import logging
import os
from typing import Dict, List, Union

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


find_file_cache: Dict[str, List[str]] = {}

# Setting this variable to a path will allow the module to store previously found results in a cache saved to disk. So
# even after unloading and reloading the module, finding the target will be very quick.
find_file_persistent_cache = None


def get_most_recent_file(paths: List[str]) -> Union[str, None]:
    msg = ""
    if paths:
        msg = f"Multiple {paths[0].split(os.path.sep)[-1]} files found\n"
        for p in paths:
            msg += f"  {p}\n"
        msg += f"Returning file with most recent modification date:\n"

    path_modtimes = [(p, os.path.getmtime(p)) for p in paths]

    # Sort by modification time, last modified is first
    path_modtimes.sort(key=lambda elem: elem[1], reverse=True)

    if path_modtimes:
        if len(path_modtimes) > 1:
            msg += f"  {path_modtimes[0][0]}"
            logger.warning(msg)
        return path_modtimes[0][0]

    return None



def find_file(
    target: str, root: str, search_constraint=[], cache_result=True,
) -> List[str]:
    """
    Find a file. Does not search in recycle bin.

    :param target: An exact filename that must be matched.
    :param root: The directory from which the search starts. Subdirectories will also be searched.
    :param search_constraint: Partial strings that must match subdirectory names.
                              E.g. search_constraint = ["Progr", "Vis"] will match "root/Program Files/Visual Studio/..."
                              but not "root/Tools/Program Files/Visual Studio"
    :param cache_result:      If a result has been returned for a given set of parameters, the same result will be
                              returned, and the search will be omitted. Checks if the returned files are valid.
    :return: list of paths matching target filename
    """
    if find_file_persistent_cache is not None:
        if os.path.isfile(find_file_persistent_cache):
            with open(find_file_persistent_cache, "r") as cache_file:
                try:
                    cache = json.load(cache_file)
                    logger.debug(
                        f"[find_file]: Loading values from cache file {find_file_persistent_cache}"
                    )
                    for k, v in cache.items():
                        find_file_cache[k] = v
                except:
                    pass

    if cache_result:
        if target in find_file_cache.keys():
            preferred = find_file_cache[target]

            if all([os.path.isfile(possible_match) for possible_match in preferred]):
                logger.info(f"[find_file]: Returning cached result {preferred}")
                return preferred

    result = []
    root_level = True
    for dirpath, dirnames, filenames in os.walk(root):
        if root_level:
            dirnames[:] = [d for d in dirnames if "Recycle.Bin" not in d]
            if search_constraint:
                dirnames[:] = [d for d in dirnames if search_constraint[0] in d]
        elif len(search_constraint) > (dirpath.count(os.path.sep)):
            dirnames[:] = [
                d
                for d in dirnames
                if search_constraint[dirpath.count(os.path.sep)] in d
            ]
        root_level = False

        for filename in filenames:
            if filename == target:
                abspath = os.path.abspath(os.path.join(dirpath, filename))
                result.append(abspath)

    if cache_result and result:
        find_file_cache[target] = result

        if find_file_persistent_cache is not None:
            with open(find_file_persistent_cache, "w") as cache_file:
                json.dump(find_file_cache, cache_file)

    return result

class Cache:
    def __init__(self, fullpath):
        self.data = {}
        self._fullpath = fullpath

        if os.path.isfile(fullpath):
            with open(fullpath, "r") as cache_file:
                try:
                    self.data = json.load(cache_file)
                except:
                    pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        with open(self._fullpath, "w") as cache_file:
            json.dump(self.data, cache_file, sort_keys=True, indent=4)

    def get_or_else(self, key, default_value):
        if key not in self.data:
            self.data[key] = default_value
        return self.data[key]
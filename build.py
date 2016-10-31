#!/usr/bin/env python

import errno
import glob
import os
import subprocess
import sys

CONFIGS = {}
script_dir = os.path.dirname(__file__)
for config_file in sorted(glob.glob(os.path.join(script_dir, "*build_configs.py"))):
    with open(config_file) as f:
        config_file_content = f.read()
        exec(config_file_content, globals(), CONFIGS)

DEFAULT_CONFIG_NAME = CONFIGS.pop("DEFAULT")
DEBUG_CONFIG_NAME = CONFIGS.pop("DEBUG")
                            


CMAKE = "cmake"
if os.name == "posix":
    MAKE = "make"
    CMAKE_GENERATOR = "Unix Makefiles"
elif os.name == "nt":
    MAKE = "nmake"
    CMAKE_GENERATOR = "NMake Makefiles"
else:
    print("Unsupported OS: " + os.name)
    sys.exit(1)

def get_project_root_path():
    import __main__
    return os.path.dirname(__main__.__file__)


def get_builds_path():
    return os.path.join(get_project_root_path(), "builds")


def get_src_path():
    return os.path.join(get_project_root_path(), "src")


def get_build_path(config_name):
    return os.path.join(get_builds_path(), config_name)


def build(config_name, cmake_parameters, make_parameters):
    print("Building configuration " + config_name)
    build_path = get_build_path(config_name)
    rel_src_path = os.path.relpath(get_src_path(), build_path)
    try:
        os.makedirs(build_path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(build_path):
            pass
        else:
            raise

    subprocess.check_call([CMAKE, "-G", CMAKE_GENERATOR]
                          + cmake_parameters
                          + [rel_src_path],
                          cwd=build_path)
    subprocess.check_call([MAKE] + make_parameters, cwd=build_path)
    print("Built configuration " + config_name + " successfully")


def main():
    config_names = set()
    make_parameters = []
    for arg in sys.argv[1:]:
        if arg == "--help" or arg == "-h":
            print_usage()
            sys.exit(0)
        elif arg == "--debug":
            config_names.add(DEBUG_CONFIG_NAME)
        elif arg == "--all":
            config_names |= set(CONFIGS.keys())
        elif arg in CONFIGS:
            config_names.add(arg)
        else:
            make_parameters.append(arg)
    if not config_names:
        config_names.add(DEFAULT_CONFIG_NAME)
    for config_name in config_names:
        build(config_name, CONFIGS[config_name], make_parameters)


if __name__ == "__main__":
    main()

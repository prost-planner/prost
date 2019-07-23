#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import shutil
import subprocess
import sys

def main():
    search_params = []
    run_debug = False
    for arg in sys.argv[1:]:
        if arg == "--debug":
            run_debug = True
        elif arg == "--release":
            run_debug = False
        else:
            search_params.append(arg)

    if run_debug:
        parser_name = "rddl-parser-debug"
        search_name = "search-debug"
        parser_file = "builds/debug/rddl_parser/rddl-parser"
        search_file = "builds/debug/search/search"
    else:
        parser_name = "rddl-parser-release"
        search_name = "search-release"
        parser_file = "builds/release/rddl_parser/rddl-parser"
        search_file = "builds/release/search/search"

    shutil.copy2(parser_file, "./" + parser_name)
    shutil.copy2(search_file, "./" + search_name)

    print "./" + search_name + " " + " ".join(search_params)
    exitcode = subprocess.call("./" + search_name + " " + " ".join(search_params), shell=True)

    os.remove("./" + parser_name)
    os.remove("./" + search_name)
    sys.exit(exitcode)

if __name__ == "__main__":
    main()

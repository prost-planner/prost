#! /usr/bin/env python
# -*- coding: latin-1 -*-

# Starts both the rddl_prefix_parser and the search component for the given
# problem. Note that the parser output is only temporary and removed at the end
# of this script's runtime. Therefore, if you are interested in debugging /
# developing the search component of PROST, it might be better to create all
# rddl_prefix_parser output files once with the parse.py script that is also in
# this folder.

import os
import sys
from shutil import copy2

if __name__ == "__main__":
    if len(sys.argv) < 3 or len(sys.argv) > 5:
        if os.path.isfile("../src/search/prost"):
            copy2("../src/search/prost", "./prost")
        os.system("./prost")
        print >> sys.stderr, "Usage of plan.py: ./plan.py instance \"config\" [hostname] [port]"
        exit()

    hostname = "localhost";
    port = "2323"

    if len(sys.argv) > 4:
        port = sys.argv[4]

    if len(sys.argv) > 3:
        hostname = sys.argv[3]

    instance = sys.argv[1]
    config = sys.argv[2]

    if os.path.isfile("../src/search/prost"):
        copy2("../src/search/prost", "./prost")

    if os.path.isfile("../src/rddl_parser/rddl-parser"):
        copy2("../src/rddl_parser/rddl-parser", "./rddl-parser")        
    
    os.system("./prost " + instance + " -h " + hostname + " -p " + port + " " + config)

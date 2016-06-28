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

if __name__ == "__main__":
    if len(sys.argv) < 4 or len(sys.argv) > 6:
        print >> sys.stderr, "Usage: ./plan.py domain instance config [hostname] [port]"
        exit()

    hostname = "localhost";
    port = "2323"

    if len(sys.argv) > 5:
        port = sys.argv[5]

    if len(sys.argv) > 4:
        hostname = sys.argv[4]

    domain = sys.argv[1]
    instance = sys.argv[2]
    config = sys.argv[3]

    parserOut = instance.split("/")
    parserOut = parserOut[len(parserOut)-1].split(".")[0]

    os.system("../src/rddl_prefix_parser/rddl-parser " + domain + " " + instance + " ./ -s 1")
    os.system("../src/search/prost " + parserOut + " -h " + hostname + " -p " + port + " [PROST -s 1 -se [" + config + "]]")
    os.system("rm -rf " + parserOut)

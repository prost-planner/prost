#! /usr/bin/env python
# -*- coding: latin-1 -*-

# Starts the preprocess component (src/rddl-parser) of all domain / instance
# pairs that are found in the rddl_prefix subfolder of the given directory, and
# stores them in the 'prost' subfolder.

import os
import sys

if __name__ == "__main__":
    if len(sys.argv) <2 or len(sys.argv) > 3:
        print >> sys.stderr, "Usage: ./parse.py <directory> [options]"
        exit()

    if "rddl" not in os.listdir(sys.argv[1]):
        print >> sys.stderr, "The given directory must contain a directory called rddl."
        exit()

    if len(sys.argv) == 3:
        parserArgs = sys.argv[2]
    else:
        parserArgs = "-s 1"

    files = os.listdir(sys.argv[1]+"/rddl")

    domainFiles = list()

    for f in files:
        if f.endswith("_mdp.rddl"):
            domainFiles.append(f)

    instances = dict()
    for f in files:
        if not f.endswith("_mdp.rddl"):
            domName = f.split("_inst_mdp")[0] + "_mdp.rddl"
            if domName not in domainFiles:
                print >> sys.stderr, ("No domain file for instance " + f + ".")
                exit()
            instances[f] = domName

    os.system("rm -rf " + sys.argv[1] + "/prost")
    os.system("mkdir " + sys.argv[1] + "/prost")
    for inst in instances:
        os.system("../src/rddl_parser/rddl-parser " + sys.argv[1] + "/rddl/" + instances[inst] + " " + sys.argv[1] + "/rddl/" + inst + " " + sys.argv[1] + "/prost/ " + parserArgs)


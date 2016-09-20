#! /usr/bin/env python
# -*- coding: latin-1 -*-

# Starts the preprocess component (src/rddl-parser) of all domain / instance
# pairs that are found in the rddl_prefix subfolder of the given directory, and
# stores them in the 'prost' subfolder.

import os
import sys

if __name__ == "__main__":
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print >> sys.stderr, "Usage: ./parse.py <targetDir> [options]"
        exit()

    targetDir = sys.argv[1]
    dirs = os.listdir(targetDir)
    if "rddl_prefix" not in dirs:
        print >> sys.stderr, "Directory targetDir must contain a directory called rddl_prefix."
        exit()

    if len(sys.argv) == 3:
        parserArgs = sys.argv[2]
    else:
        parserArgs = "-s 1"

    files = os.listdir(targetDir+"/rddl_prefix")
    domainFiles = list()
    for f in files:
        if f.endswith("_mdp.rddl_prefix"):
            domainFiles.append(f)

    instances = dict()
    for f in files:
        if not f.endswith("_mdp.rddl_prefix"):
            domName = f.split("_inst_mdp")[0] + "_mdp.rddl_prefix"
            if domName not in domainFiles:
                print >> sys.stderr, ("No domain file for instance " + f + ".")
                exit()
            instances[f] = domName

    os.system("rm -rf " + targetDir + "/prost")
    os.system("mkdir " + targetDir + "/prost")
    for inst in instances:
        os.system("../src/rddl_parser/rddl-parser " + targetDir + "/rddl_prefix/" + instances[inst] + " " + targetDir + "/rddl_prefix/" + inst + " " + targetDir + "/prost/ " + parserArgs)


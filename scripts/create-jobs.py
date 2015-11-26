#! /usr/bin/env python
# -*- coding: utf-8 -*-

from Cheetah.Template import Template
import os
import sys

# defines the benchmark that is used for the experiment (must be the name of a
# folder in testbed/benchmarks)
benchmark="ippc-all"

# defines which queue to use for one task. Possible values are
# "athlon.q" and "athlon_core.q". The former value configures the use
# of a whole cpu, while the latter option configures the use of a
# single cpu core.
queue = "all.q@ase*"

# defines the timeout for one taks. The time format is
# "hours:minutes:seconds", eg, a value of "0:30:00" sets the timeout
# to 30 minutes. If timout is set to None, then there is no timeout.
timeout = None

# defines the maximum amount of available memory for one task. The
# value's format is either "<mem>M" or "<mem>G", where <mem> is an
# integer number, M stands for MByte and G for GByte. If memout is
# None, then there is no memory bound.
memout = None

revision = "rev144"

configs = [
    "[IPPC2011]",
    "[IPPC2014]",

    "[UCTStar -i [IDS]]",
    "[UCTStar -i [MLS]]",
    "[UCTStar -i [Uniform]]",
    "[UCTStar -i [RandomWalk]]",

    "[DP-UCT -i [IDS]]",
    "[DP-UCT -i [MLS]]",
    "[DP-UCT -i [Uniform]]",
    "[DP-UCT -i [RandomWalk]]",

    "[MC-UCT -i [IDS]]",
    "[MC-UCT -i [MLS]]",
    "[MC-UCT -i [Uniform]]",
    "[MC-UCT -i [RandomWalk]]",

    "[MaxUCT -i [IDS]]",
    "[MaxUCT -i [MLS]]",
    "[MaxUCT -i [Uniform]]",
    "[MaxUCT -i [RandomWalk]]",
]

host = "localhost"
resultsDir = "results/"+revision+"/"+revision+"_"
numRuns = "100"

TASK_TEMPLATE = "export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH && " \
"mkdir -p %(resultsDir)s && " \
"./run-server benchmarks/%(benchmark)s/ %(port)s %(numRuns)s > %(resultsDir)s/%(instance)s_server.log 2> %(resultsDir)s/%(instance)s_server.err &" \
" sleep 45 &&" \
" ./prost benchmarks/%(benchmark)s/prost/%(instance)s -p %(port)s [PROST -s 1 -se %(config)s] > %(resultsDir)s/%(instance)s.log 2> %(resultsDir)s/%(instance)s.err"

def isInstanceName(fileName):
    return fileName.count("inst") > 0

def create_tasks(filename, instances):
    port = 2000
    tasks = []

    print len(instances)
    print instances
    
    for config in configs:
        for instance in sorted(instances):
            task = TASK_TEMPLATE % dict(config=config, benchmark = benchmark, instance=instance, host=host,
                                        port=port, numRuns = numRuns, resultsDir=resultsDir+config.replace(" ","_"))
            tasks.append(task)
            port = port + 1


    template = Template(file='job.tmpl',
                        searchList=[{'tasks'   : tasks,
                                     'queue'   : queue,
                                     'timeout' : timeout,
                                     'memout'  : memout}])    
    f = file(filename, 'w')
    f.write(str(template))
    f.close()

if __name__ == '__main__':
    if len(sys.argv) > 1:
        print >> sys.stderr, "Usage: create-jobs.py"
        exit()
    instances = filter(isInstanceName, os.listdir("../testbed/benchmarks/"+benchmark+"/rddl/"))
    instances = [instance.split(".")[0] for instance in instances]
    create_tasks("../testbed/prost.q", instances)

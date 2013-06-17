#! /usr/bin/env python
# -*- coding: utf-8 -*-

from Cheetah.Template import Template
import os
import sys

# the default benchmark directory that is used if none is given as
# parameter
benchmarkDir="../testbed/benchmarks/ippc2011/rddl/"

# defines which queue to use for one task. Possible values are
# "athlon.q" and "athlon_core.q". The former value configures the use
# of a whole cpu, while the latter option configures the use of a
# single cpu core. queue = "opteron_core.q"
queue = "opteron_core.q"

# defines the timeout for one taks. The time format is
# "hours:minutes:seconds", eg, a value of "0:30:00" sets the timeout
# to 30 minutes. If timout is set to None, then there is no timeout.
timeout = None

# defines the maximum amount of available memory for one task. The
# value's format is either "<mem>M" or "<mem>G", where <mem> is an
# integer number, M stands for MByte and G for GByte. If memout is
# None, then there is no memory bound.
memout = None

revision = "rev19"

configs = [
    "[IDS]",
    "[MC-UCT -i [IDS]]",
    "[IPPC2011]",
    "[DP-UCT -i [IDS]]",    
    "[UCTStar -i [IDS]]",
    "[MaxMC-UCT -i [IDS]]",
    #"[Uniform]",
    #"[MC-UCT -i [Uniform]]",
    #"[MC-UCT -sd 15 -i [Uniform]]",
    #"[DP-UCT -i [Uniform]]",    
    #"[UCTStar -i [Uniform]]",
    #"[MaxMC-UCT -i [Uniform]]",
]

host = "localhost"
baseDir = "benchmarks/ippc2011"

resultsDir = "results/"+revision+"/"+revision+"_"

TASK_TEMPLATE = "export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH && " \
"mkdir -p %(resultsDir)s && " \
"./run-server benchmarks/ippc2011/rddl/ %(port)d 100 > %(resultsDir)s/%(instance)s_server.log 2> %(resultsDir)s/%(instance)s_server.err &" \
" sleep 30 &&" \
" ./prost %(baseDir)s/rddl_prefix/ %(instance)s -p %(port)s [PROST -s 1 -se %(config)s] > %(resultsDir)s/%(instance)s.log 2> %(resultsDir)s/%(instance)s.err"

def isInstanceName(fileName):
    return fileName.count("inst") > 0

def create_tasks(filename, instances):
    port = 2000
    tasks = []

    print len(instances)
    print instances
    
    for config in configs:
        for instance in sorted(instances):
            task = TASK_TEMPLATE % dict(config=config, instance=instance, host=host, port=port, baseDir=baseDir, resultsDir=resultsDir+config.replace(" ","_"))
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
    if len(sys.argv) > 2:
        print >> sys.stderr, "Usage: create-jobs.py [benchmarkDir]"
        exit()
    if len(sys.argv) == 2:
        benchmarkDir = sys.argv[1]
    instances = filter(isInstanceName, os.listdir(benchmarkDir))
    instances = [instance.split(".")[0] for instance in instances]
    create_tasks("../testbed/prost.q", instances)

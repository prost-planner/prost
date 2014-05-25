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
# single cpu core.
queue = "meta_core.q"

# defines the timeout for one taks. The time format is
# "hours:minutes:seconds", eg, a value of "0:30:00" sets the timeout
# to 30 minutes. If timout is set to None, then there is no timeout.
timeout = None

# defines the maximum amount of available memory for one task. The
# value's format is either "<mem>M" or "<mem>G", where <mem> is an
# integer number, M stands for MByte and G for GByte. If memout is
# None, then there is no memory bound.
memout = None

revision = "rev99"

configs = [
    "DP-UCT -iv 1"

    #"DP-UCT -ndn 1 -hw 0.5",
    #"DP-UCT -ndn 1 -iv 1 -hw 0.5",
    #"DP-UCT -ndn 1 -hw 1.0",
    #"DP-UCT -ndn 1 -iv 1 -hw 1.0",
    #"DP-UCT -ndn 1 -hw 3.0",
    #"DP-UCT -ndn 1 -iv 1 -hw 3.0",

    #"DP-UCT -ndn 2 -hw 0.1",
    #"DP-UCT -ndn 2 -iv 1 -hw 0.1",
    #"DP-UCT -ndn 2 -hw 0.5",
    #"DP-UCT -ndn 2 -iv 1 -hw 0.5",
    #"DP-UCT -ndn 2 -hw 1.0",
    #"DP-UCT -ndn 2 -iv 1 -hw 1.0",
    #"DP-UCT -ndn 2 -hw 3.0",
    #"DP-UCT -ndn 2 -iv 1 -hw 3.0",

    #"DP-UCT -ndn 4 -hw 0.1",
    #"DP-UCT -ndn 4 -iv 1 -hw 0.1",
    #"DP-UCT -ndn 4 -hw 0.5",
    #"DP-UCT -ndn 4 -iv 1 -hw 0.5",
    #"DP-UCT -ndn 4 -hw 1.0",
    #"DP-UCT -ndn 4 -iv 1 -hw 1.0",
    #"DP-UCT -ndn 4 -hw 3.0",
    #"DP-UCT -ndn 4 -iv 1 -hw 3.0",

    #"DP-UCT -ndn 8 -hw 0.1",
    #"DP-UCT -ndn 8 -iv 1 -hw 0.1",
    #"DP-UCT -ndn 8 -hw 0.5",
    #"DP-UCT -ndn 8 -iv 1 -hw 0.5",
    #"DP-UCT -ndn 8 -hw 1.0",
    #"DP-UCT -ndn 8 -iv 1 -hw 1.0",
    #"DP-UCT -ndn 8 -hw 3.0",
    #"DP-UCT -ndn 8 -iv 1 -hw 3.0",

    #"DP-UCT -ndn 1 -sd 10",
    #"DP-UCT -ndn 1 -sd 15",
    #"DP-UCT -ndn 1 -sd 20",

    #"DP-UCT",
    #"DP-UCT -sd 10",
    #"DP-UCT -sd 15",
    #"DP-UCT -sd 20",

    #"MC-UCT",
    #"MC-UCT -sd 10",
    #"MC-UCT -sd 15",
    #"MC-UCT -sd 20",

    #"[MaxMC-UCT -i [IDS]]",
    #"[BFS -i [IDS]]",
    #"[IPPC2011]",
    #"[IDS]",
    #"[Uniform]",
    #"[MC-UCT -i [Uniform]]",
    #"[MC-UCT -sd 15 -i [Uniform]]",
    #"[DP-UCT -i [Uniform]]",    
    #"[UCTStar -i [Uniform]]",
    #"[MaxMC-UCT -i [Uniform]]",
]

#complete_configs = [
#    "-tm UNI -se [BFS -mv 0 -i [IDS]]",
#    "-tm MAN -se [BFS -mv 0 -i [IDS]]",
#    "-tm UNI -se [BFS -mv 0 -i [MLS]]",
#    "-tm MAN -se [BFS -mv 0 -i [MLS]]",
#    "-tm UNI -se [BFS -mv 1 -i [IDS]]",
#    "-tm MAN -se [BFS -mv 1 -i [IDS]]",
#    "-tm UNI -se [BFS -mv 1 -i [MLS]]",
#    "-tm MAN -se [BFS -mv 1 -i [MLS]]"
#]

#timeout_meth = [ "UNI" ]#, "MAN" ]
#init = [ "IDS" ]#, "MLS" ]
most_visits = [ "0", "1" ]
magic_constant = [ "1.0", "1.5", "0.5", "2.5", "0.2" ]

heuristic_weight = [ "0.5", "1.0", "1.5", "2.0" ]
new_dec_nodes = [ "1", "2", "4", "8", "16" ]
#expl_mode = [ "LOG", "SQRT", "LNQUAD", "LIN" ]
#least_visit_in_root = [ "0", "1" ]



host = "localhost"
baseDir = "benchmarks/ippc2011"

resultsDir = "results/"+revision+"/"+revision+"_"

TASK_TEMPLATE = "export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH && " \
"mkdir -p %(resultsDir)s && " \
"./run-server benchmarks/ippc2011/rddl/ %(port)s > %(resultsDir)s/%(instance)s_server.log 2> %(resultsDir)s/%(instance)s_server.err &" \
" sleep 45 &&" \
" ./prost domains/%(instance)s -p %(port)s [PROST -s 1 -tm UNI -se [%(config)s -i [IDS]]] > %(resultsDir)s/%(instance)s.log 2> %(resultsDir)s/%(instance)s.err"

def isInstanceName(fileName):
    return fileName.count("inst") > 0

def create_tasks(filename, instances):
    port = 2000
    tasks = []

    print len(instances)
    print instances
    
    #for config in configs:
    for most_visits_val in most_visits:
        for magic_constant_val in magic_constant:
            for h_val in heuristic_weight:
                for ndn in new_dec_nodes:
                    #for expl_mode_val in expl_mode:
                    #for least_visit_in_root_val in least_visit_in_root:
                    for instance in sorted(instances):
                        conf = "DP-UCT -iv 1 -ndn %s -hw %s -mcs %s -mv %s" %(ndn, h_val, magic_constant_val, most_visits_val)
                        task = TASK_TEMPLATE % dict(config=conf, instance=instance, host=host, port=port, resultsDir=resultsDir+conf.replace(" ","_"))
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

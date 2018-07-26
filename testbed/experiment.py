#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys

############ BASEL GRID PARAMETER ############

# Load "infai" settings for partition and qos 
partition="infai"
qos="infai"

# Gives the task's priority as a value between 0 (highest) and 2000 (lowest).
nice="2000"

# The email adress that receives an email when the experiment is finished
email = "tho.keller@unibas.ch"

############ FREIBURG GRID PARAMETER ############
# defines which queue to use for one task. Possible values are
# "athlon.q" and "athlon_core.q". The former value configures the use
# of a whole cpu, while the latter option configures the use of a
# single cpu core.
queue = "meta_core.q"

# defines the priority of the task. Possible values are [-1023,0], but the
# maximum of 0 should only be used for very urgent jobs.
priority = 0

############ OTHER PARAMETERS ############

# Available options are "slurm" and "sge" (for sun grid engine)
grid_engine = "slurm"

# The benchmark that is used for the experiment (must be the name of a
# folder in testbed/benchmarks)
benchmark="ippc-all"

# The search engine configurations that are started in this experiment
# (each of these is run on each instance in the benchmark folder)
configs = [
    "IPPC2011",                                         # The configuration that participated at IPPC 2011
    "IPPC2014",                                         # The configuration that participated at IPPC 2014
    "UCT -init [Single -h [RandomWalk]]",               # The configuration that is closest to "plain UCT"
    "UCT -init [Expand -h [IDS]] -rec [MPA]",           # Best UCT configuration according to Keller's dissertation
    "DP-UCT -init [Single -h [Uniform]]"                # A configuration that works well in wildfire and sysadmin    
]

# The number of runs (30 in competition, should be higher (>=100) for
# papers and theses to obtain acceptable confidence intervalls
numRuns = "100"

# The current revision (used for appropriate naming only)
revision = "rev3b168b35"

# The timeout per task in hh:mm:ss
timeout = "1:40:00"

# The maximum amount of available memory per task. The value's format is
# either "<mem>M" or "<mem>G", where <mem> is an integer number, M
# stands for MByte and G for GByte. Note that PROST additionally has an
# internal memory management that makes sure that the used memory that
# not exceed a given value (see src/search/prost_planner.cc)
memout = "3872M"

############ (USUALLY) NO NEED TO CHANGE THE FOLLOWING ############

# The experiment's name
name = "prost_"+revision

# Directory results are written to
resultsDir = "results/"+revision+"/"

# Directory server logs are written to
serverLogDir = resultsDir+"serverLogs/"

# The file where stderr is directed to
errfile = "stderr.log"

# The file where stdout is directed to
logfile = "stdout.log"

# Template for the string that is executed for each job
TASK_TEMPLATE = "export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH && " \
"mkdir -p %(resultsDir)s && " \
"./run-server benchmarks/%(benchmark)s/ %(port)s %(numRuns)s 0 1 0 %(serverLogDir)s > %(resultsDir)s/%(instance)s_server.log 2> %(resultsDir)s/%(instance)s_server.err &" \
" sleep 45 &&" \
" ../src/search/prost benchmarks/%(benchmark)s/prost/%(instance)s -p %(port)s [PROST -s 1 -se [%(config)s]] > %(resultsDir)s/%(instance)s.log 2> %(resultsDir)s/%(instance)s.err"

SLURM_TEMPLATE = "#! /bin/bash -l\n" \
                 "### Set name.\n"\
                 "#SBATCH --job-name=%(name)s\n"\
                 "### Redirect stdout and stderr.\n"\
                 "#SBATCH --error=%(errfile)s\n"\
                 "#SBATCH --output=%(logfile)s\n"\
                 "### Set partition.\n"\
                 "#SBATCH --partition=%(partition)s\n"\
                 "### Set quality-of-service group.\n"\
                 "#SBATCH --qos=%(qos)s\n"\
                 "### Set memory limit\n"\
                 "#SBATCH --mem-per-cpu=%(memout)s\n"\
                 "### Set timeout\n"\
                 "#SBATCH -t %(timeout)s\n"\
                 "### Number of tasks.\n"\
                 "#SBATCH --array=1-%(num_tasks)s\n"\
                 "### Adjustment to priority ([-2147483645, 2147483645]).\n"\
                 "#SBATCH --nice=%(nice)s\n"\
                 "### Send mail? Mail type can be e.g. NONE, END, FAIL, ARRAY_TASKS.\n"\
                 "#SBATCH --mail-type=END\n"\
                 "#SBATCH --mail-user=%(email)s\n"\
                 "### Extra options.\n\n"

SGE_TEMPLATE = "#! /bin/bash\n"\
               "## specifies the interpreting shell for this job file.\n"\
               "#$ -S /bin/bash\n"\
               "## Never send me an email.\n"\
               "#$ -m n\n"\
               "## Execute the job from the current working directory.\n"\
               "#$ -cwd\n"\
               "## stderr and stdout go here\n"\
               "#$ -e %(errfile)s\n"\
               "#$ -o %(logfile)s\n"\
               "## set time out\n"\
               "#$ -l h_cpu=%(timeout)s\n"\
               "## set memory out\n"\
               "#$ -l h_vmem=%(memout)s\n"\
               "## use this queue. \n"\
               "#$ -q %(queue)s\n"\
               "## the number of tasks is this job file\n"\
               "#$ -t 1-%(num_tasks)s\n"\
               "## the priority of this job.\n"\
               "#$ -p %(priority)s\n\n"

def isInstanceName(fileName):
    return fileName.count("inst") > 0

def create_tasks(filename, instances):
    port = 2000
    tasks = []
    
    for config in configs:
        for instance in sorted(instances):
            task = TASK_TEMPLATE % dict(config=config,
                                        benchmark = benchmark,
                                        instance=instance,
                                        port=port,
                                        numRuns = numRuns,
                                        resultsDir=resultsDir+config.replace(" ","_"),
                                        serverLogDir=serverLogDir)
            tasks.append(task)
            port = port + 1

    if grid_engine == "slurm":
        jobs = SLURM_TEMPLATE %dict(name=name,
                                    errfile=errfile,
                                    logfile=logfile,
                                    partition=partition,
                                    qos=qos,
                                    memout=memout,
                                    timeout=timeout,
                                    num_tasks=str(len(tasks)),
                                    nice=nice,
                                    email=email)
        
        for task_id,task in zip(range(1, len(tasks)+1), tasks):
            jobs += "if [ " + str(task_id) + " -eq $SLURM_ARRAY_TASK_ID ]; then\n"
            jobs += "    " + task + "\n"
            jobs += "    exit $?\n"
            jobs += "fi\n"
    elif grid_engine == "sge":
        jobs = SGE_TEMPLATE %dict(errfile=errfile,
                                  logfile=logfile,
                                  memout=memout,
                                  timeout=timeout,
                                  queue=queue,
                                  num_tasks=str(len(tasks)),
                                  priority=str(priority))
        
        for task_id,task in zip(range(1, len(tasks)+1), tasks):
            jobs += "if [ " + str(task_id) + " -eq $SGE_TASK_ID ]; then\n"
            jobs += "    " + task + "\n"
            jobs += "    exit $?\n"
            jobs += "fi\n"
    else:
        print "Invalid grid engine!"
        exit()

    f = file(filename, 'w')
    f.write(str(jobs))
    f.close()

def run_experiments(filename):
    if grid_engine == "slurm":
        os.system("sbatch " + filename + " &")
    elif grid_engine == "sge":
        os.system("qsub " + filename + " &")
    else:
        print "Invalid grid engine!"
        exit() 

if __name__ == '__main__':
    if len(sys.argv) > 1:
        print >> sys.stderr, "Usage: create-jobs.py"
        exit()
    instances = filter(isInstanceName, os.listdir("../testbed/benchmarks/"+benchmark+"/rddl/"))
    instances = [instance.split(".")[0] for instance in instances]
    os.system("mkdir -p " + resultsDir)
    os.system("mkdir -p " + serverLogDir)
    filename = resultsDir + "experiment_"+revision
    create_tasks(filename, instances)
    run_experiments(filename)

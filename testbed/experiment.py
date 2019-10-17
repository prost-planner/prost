#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import shutil
import subprocess
import sys
import json

from benchmark_suites import *

############ SLURM PARAMETER ############

# Load "infai" settings for partition and qos
#partition = 'infai_2'
partition = 'gki_cpu-ivy'
qos = 'normal'

# The email adress that receives an email when the experiment is finished
email = 'my.name@unibas.ch'

############ OTHER PARAMETERS ############

# The current revision (used for appropriate naming only). You can also
# set this manually
out = subprocess.check_output(['hg', 'log','-l1']).splitlines()
revision = out[0].split(':')[2].strip()

#revision = 'rev3b168b35'
# Set to true if you want to run the experiment in debug mode
run_debug = False

# A list of domains that are used in this experiment. Each entry must correspond
# to a folder in testbed/benchmarks. See testbed/benchmark_suites.py for some
# predefined benchmark sets, such as IPC2018 or IPC_ALL.
benchmark = IPC_ALL

# The search engine configurations that are started in this experiment.
# (each of these is run on each instance in the benchmark folder)
configs = [
    'IPPC2011',                                        # The configuration that participated at IPC 2011
    'IPPC2014',                                        # The configuration that participated at IPC 2014
   #'UCT -init [Single -h [RandomWalk]]',              # The configuration that is closest to "plain UCT"
   #'UCT -init [Expand -h [IDS]] -rec [MPA]',          # Best UCT configuration according to Keller's dissertation
   #'DP-UCT -init [Single -h [Uniform]]'               # A configuration that works well in wildfire and sysadmin
]

# The number of runs (30 in competition, should be higher (>=100) for
# papers to obtain acceptable confidence in the results
num_runs = '100'

# Average time per step which depends on the horizion and number of runs
# As a rule of thumb, we provide 75*2.5*H seconds for each instance 
# (exceptions are possible), where H is the finite horizon. As a result 
# of a discussion among the participants and organizers, we decided to 
# increase the average deliberation time significantly (this was 50*H seconds before). 
step_time = '2.5'

# Time to wait for the rddl server to setup in sec
sleep_time = '300'


# The timeout per task in hh:mm:ss
# Should be creater than max(horizon) * num_runs * step_time + sleep_time
# Default this is 120 * 100 * 2.5s + 300s = 30300s = 505min ~ 8.5h
timeout = '9:00:00'

# The maximum amount of available memory per task. The value's format is
# either "<mem>M" or "<mem>G", where <mem> is an integer number, M
# stands for MByte and G for GByte. Note that PROST additionally has an
# internal memory management that aims to not use more memory than a
# given value (see src/search/prost_planner.cc), but that is not very
# reliable.
memout = '3872M'



############ (USUALLY) NO NEED TO CHANGE THE FOLLOWING ############

# The experiment's name
name = 'prost_' + revision

# Directory results are written to
results_dir = os.path.join('results', revision)

# Path to executable
prost_file = os.path.join(results_dir, 'prost')

# Directory server logs are written to
server_log_dir = os.path.join('results', revision, 'server_logs')

# The file where the grid engine's stderr is directed to
errfile = 'stderr.log'

# The file where the grid engine's stdout is directed to
logfile = 'stdout.log'

# Template for the string that is executed for each job
TASK_TEMPLATE = ('export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH && '
                 './run-server benchmarks/%(benchmark)s %(port)s %(num_runs)s 0 1 %(run_time)s %(server_log_dir)s 0 '
                 '> %(server_log)s '
                 '2> %(server_err)s &'
                 ' sleep %(sleep_time)s &&'
                 ' ./%(prost_file)s %(instance)s -p %(port)s [PROST -s 1 -se [%(config)s]] '
                 '> %(run_log)s '
                 '2> %(run_err)s &&'
                 'cat > %(driver_log)s')

# Template for slurm specific commands
SLURM_TEMPLATE = '''#! /bin/bash -l
### Set name.
#SBATCH --job-name=%(name)s
### Redirect stdout and stderr.
#SBATCH --error=%(errfile)s
#SBATCH --output=%(logfile)s
### Set partition.
#SBATCH --partition=%(partition)s
### Set quality-of-service group.
#SBATCH --qos=%(qos)s
### Set memory limit
#SBATCH --mem-per-cpu=%(memout)s
### Set timeout
#SBATCH -t %(timeout)s
### Number of tasks.
#SBATCH --array=1-%(num_tasks)s
### Adjustment to priority ([-2147483645, 2147483645]).
#SBATCH --nice=5000
### Send mail? Mail type can be e.g. NONE, END, FAIL, ARRAY_TASKS.
#SBATCH --mail-type=NONE
#SBATCH --mail-user=%(email)s
### Extra options.
'''

def build_planner():
    try:
        if run_debug:
            subprocess.check_call(['../build.py', '--debug'])
        else:
            subprocess.check_call(['../build.py'])
    except subprocess.CalledProcessError as e:
        print './build.py failed!'
        print 'PROST planner could not be compiled.'
    return

def copy_binaries():
    if run_debug:
        parser_name = 'rddl-parser-debug'
        parser_file = '../builds/debug/rddl_parser/rddl-parser'
        search_file = '../builds/debug/search/search'
    else:
        parser_name = 'rddl-parser-release'
        parser_file = '../builds/release/rddl_parser/rddl-parser'
        search_file = '../builds/release/search/search'

    shutil.copy2(parser_file, os.path.join('.', parser_name))
    shutil.copy2(search_file, os.path.join('.', results_dir, 'prost'))

def create_tasks(filename, instances):
    port = 2000
    tasks = []
    domains_in_benchmark = set()
    for task in benchmark:
        # The path in benchmark_suites is the directory name.
        # We use this name to identify the domains instead of the RDDL domain file.
        domains_in_benchmark.add(task.path)

    # Create properties file of the whole experiment.
    properties = dict()
    properties['domains'] = list(domains_in_benchmark)
    properties['algorithms'] = configs
    properties['memory_limit'] = memout
    properties['name'] = name
    properties['num_runs'] = num_runs
    properties['partition'] = partition
    properties['revision'] = revision
    properties['run_debug'] = run_debug
    properties['time_limit'] = timeout
    props_path = os.path.join(results_dir, 'properties')
    with open(props_path, 'w') as fp:
        json.dump(properties, fp, indent=2)

    task_id = 1
    lower = 1
    upper = 100
    for config in configs:
        for instance in sorted(instances):
            run_batch = 'runs-{:0>5}-{:0>5}'.format(lower, upper)
            run = '{:0>5}'.format(task_id)
            run_dir = os.path.join(results_dir, run_batch, run)
            if not os.path.exists(run_dir):
                os.makedirs(run_dir)

            server_log = os.path.join(run_dir, 'server.log')
            server_err = os.path.join(run_dir, 'server.err')
            run_log = os.path.join(run_dir, 'run.log')
            run_err = os.path.join(run_dir, 'run.err')
            driver_log = os.path.join(run_dir, 'driver.log')
            run_time = int(instance[2] * float(num_runs) * float(step_time))
            task = TASK_TEMPLATE % dict(config=config,
                                        benchmark =instance[0],
                                        instance=instance[1],
                                        port=port,
                                        num_runs = num_runs,
                                        run_time = run_time,
                                        prost_file=prost_file,
                                        server_log_dir=server_log_dir,
                                        server_log=server_log,
                                        server_err=server_err,
                                        sleep_time=sleep_time,
                                        run_log=run_log,
                                        run_err=run_err,
                                        driver_log=driver_log)


            # Create properties file of the run.
            # Lab requires at least the attributes with the names:
            # domain, problem and algorithm.
            properties = dict()
            properties['domain'] = instance[0]
            properties['algorithm'] = config
            properties['id'] = config, instance[0], instance[1]
            properties['problem'] = instance[1]
            properties['horizon'] = instance[2]
            properties['max_score'] = instance[4]
            properties['memory_limit'] = memout
            properties['min_score'] = instance[3]
            properties['num_runs'] = num_runs
            properties['revision'] = revision
            properties['run_dir'] = run_dir
            properties['run_time'] = run_time
            properties['time_limit'] = timeout
            props_path = os.path.join(run_dir, 'static-properties')
            with open(props_path, 'w') as fp:
                json.dump(properties, fp, indent=2)

            task_id += 1
            if task_id > upper:
                lower += 100
                upper += 100
            tasks.append(task)
            port = port + 1

    jobs = SLURM_TEMPLATE %dict(name=name,
                                errfile=errfile,
                                logfile=logfile,
                                partition=partition,
                                qos=qos,
                                memout=memout,
                                timeout=timeout,
                                num_tasks=str(len(tasks)),
                                email=email)

    for task_id,task in zip(range(1, len(tasks) + 1), tasks):
        jobs += 'if [ ' + str(task_id) + ' -eq $SLURM_ARRAY_TASK_ID ]; then\n'
        jobs += '    ' + task + '\n'
        jobs += '    exit $?\n'
        jobs += 'fi\n'

    f = file(filename, 'w')
    f.write(str(jobs))
    f.close()

def run_experiment(filename):
    subprocess.check_call(['sbatch', filename, '&'])

if __name__ == '__main__':
    if len(sys.argv) > 1:
        print >> sys.stderr, 'Usage: create-jobs.py'
        exit()

    # Create results and log directory
    subprocess.check_call(['mkdir', '-p', results_dir])
    subprocess.check_call(['mkdir', '-p', server_log_dir])

    # Compile and copy planner
    build_planner()
    copy_binaries()

    # Generate tasks
    instances = []
    for instance in benchmark:
        domain_name = instance.path
        problem_name = instance.problem.replace('.rddl','')
        problem_horizon = instance.horizon
        problem_min_score = instance.min_score
        problem_max_score = instance.max_score
        instances.append((domain_name, problem_name, problem_horizon, problem_min_score, problem_max_score))
    filename = os.path.join(results_dir, 'experiment_' + revision)
    create_tasks(filename, instances)

    # Run experiment
    run_experiment(filename)

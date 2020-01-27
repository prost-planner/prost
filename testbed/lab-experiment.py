#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import platform

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.experiment import Experiment

from downward import suites
from downward.reports.absolute import AbsoluteReport
from downward.reports.scatter import ScatterPlotReport

from benchmark_suites import *



"""
How to run:

   1. Set an environment variable PROST_ROOT to the directory
      where 'prost.py' is
"""

# Create custom report class with suitable info and error attributes.
class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = []
    ERROR_ATTRIBUTES = [
        'domain', 'problem', 'algorithm', 'unexplained_errors', 'error', 'node']


def get_run_time(horizon, num_runs, step_time):
    return int(horizon * float(num_runs) * float(step_time))

NODE = platform.node()
REMOTE = NODE.endswith(".scicore.unibas.ch") or NODE.endswith(".cluster.bc2.ch")
BENCHMARKS_DIR = os.getcwd()+"/benchmarks/"
PROST = os.environ["PROST_ROOT"]

if REMOTE:
    SUITE = IPC2014
    ENV = BaselSlurmEnvironment(
        partition='infai_2',
        memory_per_cpu="6G",
        extra_options='#SBATCH --cpus-per-task=2',
        setup="%s\n%s" % (
            BaselSlurmEnvironment.DEFAULT_SETUP,
            "export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH\n"),
        export=["PATH"])
else:
    SUITE = [Benchmark('wildfire_mdp.rddl', 'wildfire_inst_mdp__1.rddl', 'wildfire-2014', 40, -4126.83, None)]
    ENV = LocalEnvironment(processes=4)

TIME_LIMIT = 30300 # in seconds
MEMORY_LIMIT = 3872 # in MiB

ATTRIBUTES=['time',
            'total_reward']

# Create a new experiment.
exp = Experiment(environment=ENV)

# Add custom parser for Power Lifted.
exp.add_parser('parser.py')

CONFIGS = [
    'IPC2011',
    'IPC2014'
]

# Prost configs
num_runs = 100
port = 2000
sleep_time = 300
step_time = 2.5

# Create one run for each instance and each configuration
for config in CONFIGS:
    for task in SUITE:
        run = exp.add_run()
        run.add_resource('domain', task.get_domain_path(), symlink=True)
        run.add_resource('problem', task.get_problem_path(), symlink=True)
        run.add_command('run-server',
                        ['./run-server', BENCHMARKS_DIR+task.domain, port, num_runs, '0', '1', get_run_time(task.horizon, num_runs, step_time), '.', 0],
                        time_limit = TIME_LIMIT,
                        stdout = 'server.log', stderr = 'server.err')
        run.add_command('sleep', ['sleep', sleep_time])
        run.add_command('run-prost',
                        [PROST+'/prost.py', task.get_problem_path(), '-p', port, '[PROST', '-s', 1, '-se', config,']'],
                        time_limit=TIME_LIMIT,
                        memory_limit=MEMORY_LIMIT,
                        stdout='run.log', stderr='run.err')
        run.set_property('domain', task.domain)
        run.set_property('problem', task.problem)
        run.set_property('algorithm', config)
        run.set_property('id', [config, task.domain, task.problem])
        run.set_property('horizon', task.horizon)
        run.set_property('max_score', task.max_score)
        run.set_property('min_score', task.min_score)
        run.set_property('num_runs', num_runs)
        run.set_property('port', port)
        run.set_property('sleep_time', sleep_time)
        run.set_property('step_time', step_time)
        port += 1

# Add step that writes experiment files to disk.
exp.add_step('build', exp.build)

# Add step that executes all runs.
exp.add_step('start', exp.start_runs)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name='fetch')

# Make a report.
exp.add_report(
    BaseReport(attributes=ATTRIBUTES),
    outfile='report.html')

# Parse the commandline and run the specified steps.
exp.run_steps()

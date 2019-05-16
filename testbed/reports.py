#! /usr/bin/env python2.7

import glob
import os
import platform

from additional_filters import domain_as_category, improvement
from ippc_scores import IPPCScores

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.experiment import Experiment
from lab.reports import Attribute

from downward.reports.absolute import AbsoluteReport
from downward.reports.scatter import ScatterPlotReport


# Create custom report class with suitable info and error attributes.
class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = ['time_limit', 'memory_limit']

# Attributes to be displayed in the report.
ATTRIBUTES = ['ippc_score',
              'numRuns',
              'reward_step-all',
              'round_reward-all',
              'round_reward_99',
              'total_reward',
              'average_reward',
              'time',]

EXP_PATH = '/home/blaas/planners/prost-dev/testbed/results/rev3b168b35'

# TODO better error handling
if not EXP_PATH:
    print 'Please define a valid experiment path.'
    exit(1)

# Create a new experiment.
exp = Experiment(path=EXP_PATH)

# Add Prost parser.
exp.add_parser('parser.py')
exp.add_parse_again_step()

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name='fetch')

# Make a basic table report with IPPC scores.
ippc_scores = IPPCScores()

exp.add_report(
    BaseReport(attributes=ATTRIBUTES,
               filter=[ippc_scores.store_rewards,
                       ippc_scores.add_score]),
    outfile='report.html')

# Make a scatter plot report.
exp.add_report(
    ScatterPlotReport(
        attributes=["average_reward"],
        filter_algorithm=["IPPC2011", "IPPC2014"],
        xscale='linear',
        yscale='linear',
        get_category=domain_as_category,),
    outfile='scatterplot.png')

# Make a scatter plot report.
exp.add_report(
    ScatterPlotReport(
        attributes=["time"],
        filter_algorithm=["IPPC2011", "IPPC2014"],
        xscale='linear',
        yscale='linear',
    get_category=improvement),
    outfile='scatterplot2.png')

exp.run_steps()

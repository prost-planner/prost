#! /usr/bin/env python2.7

import glob
import os
import platform

from additional_filters import domain_as_category, improvement
from ipc_scores import IPCScores

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.experiment import Experiment
from lab.reports import Attribute

from downward.reports.absolute import AbsoluteReport
from downward.reports.scatter import ScatterPlotReport

from prost_plots import *


# Create custom report class with suitable info and error attributes.
class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = ['time_limit', 'memory_limit']

# Attributes to be displayed in the report.
ATTRIBUTES = [Attribute('ipc_score', min_wins=False),
              Attribute('num_runs', min_wins=False),
              Attribute('reward_step-all', min_wins=False),
              Attribute('round_reward-all', min_wins=False),
              Attribute('round_reward_99', min_wins=False),
              Attribute('total_reward', min_wins=False),
              Attribute('average_reward', min_wins=False),
              Attribute('time', min_wins=True),]

EXP_PATH = 'results/rev3b168b35'

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

# Make a basic table report with IPC scores.
ipc_scores = IPCScores()

exp.add_report(
    BaseReport(attributes=ATTRIBUTES,
               filter=[ipc_scores.store_rewards,
                       ipc_scores.add_score]),
    outfile='report.html')

# Make a scatter plot report.
exp.add_report(
    ScatterPlotReport(
        attributes=['average_reward'],
        filter_algorithm=['IPC2011', 'IPC2014'],
        xscale='linear',
        yscale='linear',
        get_category=domain_as_category,),
    outfile='scatterplot.png')

# Make a scatter plot report for IPC scores using filters.
exp.add_report(
    ScatterPlotReport(
        attributes=['ipc_score'],
        filter_algorithm=['IPC2011', 'IPC2014'],
        filter=[ipc_scores.store_rewards,
                ipc_scores.add_score],
        xscale='linear',
        yscale='linear',
        get_category=domain_as_category,),
    outfile='scatterplot-ipc-score.png')


# Make a plot of reward per round.
def plot_ippc2011(run):
    if run['domain'] == 'crossing-traffic-2011' and run['algorithm'] == 'IPPC2014':
        return True
    return False

list_plot = ListPlot(EXP_PATH)
exp.add_step('reward-per-round-plot',
             list_plot.plot_reward_per_round, [PlotProblem('crossing_traffic_inst_mdp__3', linestyle='--'),
                                                PlotProblem('crossing_traffic_inst_mdp__4', linestyle='-'),
                                                PlotAlgorithm('IPPC2014', color='b', marker='o'),
                                                PlotAlgorithm('IPPC2011', color='r', marker='*')])

exp.run_steps()

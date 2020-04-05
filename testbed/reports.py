#! /usr/bin/env python3

"""Example of report using the Lab framework and Matplotlib.

As a prerequisite, you must have Lab installed and in your PYTHONPATH.
You can find more information in the official website of Lab:

    https://lab.readthedocs.io

You can use this script to produce tables, scatter plots, and plots of list
attributes in several different formats.  You can also modify the file
'parser.py' to parse different attributes and properties of each run.
Similarly, you can extend or adapt 'prost_plots.py' to plot your data in a way
that suits better to you.

"""

import os
import sys

from additional_filters import domain_as_category
from ipc_scores import IPCScores

from lab.experiment import Experiment
from lab.reports import Attribute, arithmetic_mean

from downward.reports.absolute import AbsoluteReport
from downward.reports.scatter import ScatterPlotReport

from prost_plots import ListPlot, PlotAttribute, PlotAlgorithm, PlotDomain, PlotProblem


# Create custom report class with suitable info and error attributes.
class ProstBaseReport(AbsoluteReport):
    """Base report for Prost with information attributes that are present in any run
    and in any algorithm.

    """

    INFO_ATTRIBUTES = ["time_limit", "memory_limit"]


# Attributes to be displayed in the report.
ATTRIBUTES = [
    Attribute("ipc_score", min_wins=False, function=arithmetic_mean),
    Attribute("round_reward", min_wins=False),
    Attribute("total_reward", min_wins=False),
    Attribute("average_reward", min_wins=False),
    Attribute("total_time", min_wins=True),
    Attribute("parser_time", min_wins=True),
    Attribute("entries_prob_state_value_cache", min_wins=False),
    Attribute("buckets_prob_state_value_cache", min_wins=False),
    Attribute("entries_prob_applicable_actions_cache", min_wins=False),
    Attribute("buckets_prob_applicable_actions_cache", min_wins=False),
    Attribute("trial_initial_state", min_wins=False),
    Attribute("search_nodes_initial_state", min_wins=False),
    Attribute("entries_det_state_value_cache", min_wins=False),
    Attribute("buckets_det_state_value_cache", min_wins=False),
    Attribute("entries_det_applicable_actions_cache", min_wins=False),
    Attribute("buckets_det_applicable_actions_cache", min_wins=False),
    Attribute("entries_ids_reward_cache", min_wins=False),
    Attribute("buckets_ids_reward_cache", min_wins=False),
    Attribute("ids_avg_search_depth_initial_state", min_wins=False),
    Attribute("ids_avg_search_depth_total", min_wins=False),
    Attribute("run_dir"),
]

if len(sys.argv) < 2:
    print("Usage: ./reports.py [EXP PATH] [STEPS]\n")
    print(
        "Run the script only with a valid experiment path to see the steps in detail."
    )
    print(
        "(Note that the generic usage reported by Lab is different from the one for this script.)"
    )
    exit(1)

EXP_PATH = sys.argv[1]
# That is the hacky part: To avoid changing Lab, we *delete* the experiment path
# from the arguments list!
del sys.argv[1]

if not os.path.isdir(EXP_PATH):
    print("Please define a valid experiment path.")
    exit(1)

suffix = ""
if EXP_PATH.startswith("results/prost_"):
    suffix = "_{}".format(EXP_PATH[14:])
    if suffix[-1] == "/":
        suffix = suffix[:-1]

# Create a new experiment.
exp = Experiment(path=EXP_PATH)

# Add Prost parser.
exp.add_parser("parser.py")
exp.add_parse_again_step()

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name="fetch")

# Make a basic table report with IPC scores.
ipc_scores = IPCScores()

exp.add_report(
    ProstBaseReport(
        attributes=ATTRIBUTES, filter=[ipc_scores.store_rewards, ipc_scores.add_score]
    ),
    outfile="report{}.html".format(suffix),
)

# Make a scatter plot report.
# exp.add_report(
#     ScatterPlotReport(
#         attributes=['average_reward'],
#         filter_algorithm=['IPC2011', 'IPC2014'],
#         scale='linear',
#         get_category=domain_as_category,),
#     outfile='scatterplot.png')

# Make a scatter plot report for IPC scores using filters.
# exp.add_report(
#     ScatterPlotReport(
#         attributes=['ipc_score'],
#         filter_algorithm=['IPC2011', 'IPC2014'],
#         filter=[ipc_scores.store_rewards,
#                 ipc_scores.add_score],
#         scale='linear',
#         get_category=domain_as_category,),
#     outfile='scatterplot-ipc-score.png')


# Plot list attributes. From now on, we do not use Lab reports anymore.
# def plot_ippc2011(run):
#    """Example of filter for the list plot.

#    A filter function simply needs to receive a run as input and output True or
#    False.  The runs returning True will be considered in the plots, while the
#    ones returning False will be ignored.

#    NOTE: Filters for list plots receive only a single run as input.

#    """
#    if run['domain'] == 'crossing-traffic-2011' and run['algorithm'] == 'IPC2014':
#        return True
#    return False

# list_plot = ListPlot(EXP_PATH)
# exp.add_step('reward-per-round-plot',
#             list_plot.plot_list_attribute,
#             [PlotProblem('game_of_life_inst_mdp__1', linestyle='--'),
#              PlotAlgorithm('IPC2014', color='b', marker='o'),
#              PlotAlgorithm('IPC2011', color='r', marker='*')],
#             'round_reward-all',
#             outfile='prost-plot.pdf')

exp.run_steps()

# -*- coding: utf-8 -*-

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
    Attribute("ipc_score", min_wins=False, functions=arithmetic_mean),
    Attribute("num_runs", min_wins=False),
    Attribute("reward_step-all", min_wins=False),
    Attribute("round_reward-all", min_wins=False),
    Attribute("total_reward", min_wins=False),
    Attribute("average_reward", min_wins=False),
    Attribute("time", min_wins=True),
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
    outfile="report.html",
)

exp.run_steps()


import json
import os
import sys

import matplotlib.lines as mlines
import matplotlib.pyplot as plt

import numpy as np

from collections import defaultdict

def plot_reward_per_round(path, filter_func, legend_attribute):
    eval_path = path + '-eval/'
    with open(eval_path+'properties') as json_file:
        properties = json.load(json_file)


    lines = ["-","--","-.",":"]
    line_counter = 0
    map_attr_to_line = dict()
    values = []
    legends = []
    min_reward = float("inf")
    max_reward = -1.0 * float("inf")
    for run in properties:
        if filter_func(properties[run]):
            values.append(properties[run]['round_reward-all'])
            legends.append(properties[run][legend_attribute])
            if properties[run][legend_attribute] not in map_attr_to_line.keys():
                map_attr_to_line[properties[run][legend_attribute]] = line_counter
                line_counter = (line_counter+1) % len(lines)
            min_reward = min(properties[run]['round_reward-all'] + [min_reward])
            max_reward = max(properties[run]['round_reward-all'] + [max_reward])

    # Get number of runs from first algorithm and any run
    for run in properties:
        num_runs = int(properties[run]['numRuns'])
        break

    x = np.arange(num_runs)

    cont = 0
    for index, y in enumerate(values):
        x = np.arange(len(y))
        line_style = lines[map_attr_to_line[legends[index]]]
        plt.plot(x, y, line_style)

    handles = []
    for key, value in map_attr_to_line.iteritems():
        line = mlines.Line2D([], [], linestyle=lines[value],
                             color='black', label=key)
        handles.append(line)

    plt.legend(handles=handles, handlelength=3.5).draggable()

    plt.show()

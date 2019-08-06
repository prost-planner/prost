
import json
import os
import sys

import matplotlib.pyplot as plt
import numpy as np

from collections import defaultdict

def plot_reward_per_round(path, alg1, alg2):
    eval_path = path + '-eval/'
    with open(eval_path+'properties') as json_file:
        properties = json.load(json_file)
    values = defaultdict(list)
    min_reward = float("inf")
    max_reward = -1.0 * float("inf")
    for run in properties:
        if properties[run]['algorithm'] in [alg1, alg2]:
            values[properties[run]['algorithm']].append(properties[run]['round_reward-all'])
            min_reward = min(properties[run]['round_reward-all'] + [min_reward])
            max_reward = max(properties[run]['round_reward-all'] + [max_reward])

    # Get number of runs from first algorithm and any run
    for run in properties:
        num_runs = int(properties[run]['numRuns'])
        break

    x = np.arange(num_runs)

    for y in values[alg1]:
        x = np.arange(len(y))
        plt.plot(x, y)

    plt.show()

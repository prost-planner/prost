# -*- coding: utf-8 -*-

"""

Separate the domains according to their first IPC appearance.
All domains are in the directory 'benchmarks/'.

Domain are defined by their instances. Each instance object
contains the domain file, the problem file, the directory,
and the min- and max-scores.

Domains are listed alphabetically. IPC tracks are listed
chronologically.

(Note that some domains used in the IPC 2014 are originally
from the IPC 2011.)

"""

import os

from prostlab.suites import Problem

ACADEMIC_ADVISING_2014 = [
    Problem("academic-advising-2014", inst, -200.0,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "academic-advising-2014")
    ) for inst in range(1,11)
]


min_rewards = [-98.2, -89.5333333333, -100.0, -100.0, -100.0,
              -150.0, -150.0, -150.0, -150.0, -150.0,
              -200.0, -200.0, -200.0, -200.0, -200.0,
              -250.0, -250.0, -250.0, -250.0, -250.0,]
ACADEMIC_ADVISING_2018 = [
    Problem("academic-advising-2018", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "academic-advising-2018")
    ) for inst, min_reward in zip(range(1,21), min_rewards)
]


min_rewards = [14.7066666667, 40.4933333333, 43.7733333333, 45.2666666667,
              46.3333333333, 83.2533333333, 111.12, 34.1866666667,
              67.8933333333, 80.9066666667, 34.3733333333, 75.2533333333,
              34.0666666667, 40.3733333333, 70.3333333333, 173.653333333,
              243.146666667, 130.253333333, 134.52, 136.653333333,]
CHROMATIC_DICE_2018 = [
    Problem("chromatic-dice-2018", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "chromatic-dice-2018")
    ) for inst, min_reward in zip(range(1,21), min_rewards)
]


min_rewards = [0.179066666667, 0.310533333333, 0.0904, 0.5424, 0.187466666667,
              0.064, 0.330533333333, 0.0, 0.0, 0.502933333333,
              0.0864, 0.0, 0.154133333333, 0.128933333333, 0.0,
              0.0113333333333, 0.139733333333, 0.0, 0.0, 0.0,]
COOPERATIVE_RECON_2018 = [
    Problem("cooperative-recon-2018", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "cooperative-recon-2018")
    ) for inst, min_reward in zip(range(1,21), min_rewards)
]


min_rewards = [-32.73, -34.27, -39.03, -40.0, -38.97,
              -40.0, -39.47, -40.0, -39.17, -40.0,]
CROSSING_TRAFFIC_2011 = [
    Problem("crossing-traffic-2011", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "crossing-traffic-2011")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]

min_rewards = [-46.0533333333, -834.24, -1048.32, 2107.33333333, -1246.04,
              -402.013333333, -137.12, -629.106666667, -1751.53333333, -4145.85333333,
              -1152.6, -2394.82666667, -2473.06666667, -11515.16, -3343.22666667,
              -621.04, -2199.96, -3611.38666667, -5621.30666667, -14684.8,]
EARTH_OBSERVATION_2018 = [
    Problem("earth-observation-2018", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "earth-observation-2018")
    ) for inst, min_reward in zip(range(1,21), min_rewards)
]


min_rewards = [-65.2, -54.7, -71.7, -94.07, -111.8,
              -118.67, -129.63, -146.9, -160.37, -109.7,]
ELEVATORS_2011 = [
    Problem("elevators-2011", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "elevators-2011")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [70.53, 73.27, 107.17, 167.23, 203.07,
              224.17, 264.97, 295.23, 311.47, 176.73,]
GAME_OF_LIFE_2011 = [
    Problem("game-of-life-2011", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "game-of-life-2011")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [0.0, 120.510933333, 178.050686667, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,]
MANUFACTURER_2018 = [
    Problem("manufacturer-2018", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "manufacturer-2018")
    ) for inst, min_reward in zip(range(1,21), min_rewards)
]


min_rewards = [-37.27, -40.0, -40.0, -39.83, -40.0,
             -40.0, -40.0, -38.73, -40.0, -40.0,]
NAVIGATION_2011 = [
    Problem("navigation-2011", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "navigation-2011")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [31.1466666667, 37.5466666667, 48.5333333333, 21.6016666667, 21.300234375,
              175.707434896, 17.4816666667, 27.7428255208, 2162.97210124, 10.1816666667,
              13.45796875, 1599.48311483, 8.67, 11.2679557292, 77.8681770833,
              50.9986979167, 11.5055013021, 6.939375, 2.03479166667, 2.19208333333,]
PUSH_YOUR_LUCK_2018 = [
    Problem("push-your-luck-2018", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "push-your-luck-2018")
    ) for inst, min_reward in zip(range(1,21), min_rewards)
]

RECON_2011 = [
    Problem("recon-2011", inst, 0.0,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "recon-2011")
    ) for inst in range(1,11)
]

min_rewards = [-3233.33333, 542.8, -6624.4, -5152.0, -4486.26666667,
              -9000.66666667, 11796.1333333, -7258.8, -7496.13333333, 3910.93333333,
              -481.866666667, 1886.0, -6857.86666667, 4011.86666667, -8514.66666667,
              18367.6, 8665.6, -11896.6666667, -6029.46666667, -1241.86666667,]
RED_FINNED_BLUE_EYE_2018 = [
    Problem("red-finned-blue-eye-2018", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "red-finned-blue-eye-2018")
    ) for inst, min_reward in zip(range(1,21), min_rewards)
]


min_rewards = [39.34, 33.75, -83.23, -108.73, -285.95,
              -302.99, -382.86, -534.7, -540.62, -632.0,]
SKILL_TEACHING_2011 = [
    Problem("skill-teaching-2011", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "skill-teaching-2011")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [210.18, 166.68, 345.4, 311.79, 443.05,
              407.16, 513.74, 426.46, 612.06, 471.1,]
SYSADMIN_2011 = [
    Problem("sysadmin-2011", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "sysadmin-2011")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [-633.01, 948.98, -820.54, -1178.76, -1276.74,
              -1432.27, -1452.65, -1629.29, 1463.72, 58.88,]
TAMARISK_2014 = [
    Problem("tamarisk-2014", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "tamarisk-2014")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [-19.97, -47.87, -63.93, -119.33, -120.7,
              -156.67, -146.57, -154.37, -108.03, -243.7,]
TRAFFIC_2011 = [
    Problem("traffic-2011", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "traffic-2011")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [-32.27, 28.47, -40.0, -40.0, -40.0,
              -40.0, -40.0, -40.0, -40.0, -40.0,]
TRIANGLE_TIREWORLD_2014 = [
    Problem("triangle-tireworld-2014", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "triangle-tireworld-2014")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [-4126.83, -14146.83, -10424.5, -23154.0, -8169.83,
              -24726.33, -16646.67, -26196.67, -18122.5, -27999.67,]
WILDFIRE_2014 = [
    Problem("wildfire-2014", inst, min_reward,
            benchmarks_dir=os.path.join(os.getcwd(), "benchmarks", "wildfire-2014")
    ) for inst, min_reward in zip(range(1,11), min_rewards)
]


min_rewards = [861.6948, 772.2744, 784.024133333, 661.932, 537.096,
              701.793866667, 268.610666667, 727.618, 751.386666667, 636.292266667,
              1049.56306667, 574.419066667, 758.8136, 777.9772, 1252.9972,
              695.311866667, 1467.67613333, 547.9844, 1595.51253333, 769.4372,]
WILDLIFE_PRESERVE_2018 = [
    Problem("wildlife-preserve-2018", inst, min_reward,
            domain_file=os.path.join(os.getcwd(), "benchmarks",
                                     "wildlife-preserve-2018",
                                     "wildlife-preserve_{:02d}_mdp.rddl".format(inst)),
            problem_file=os.path.join(os.getcwd(), "benchmarks",
                                      "wildlife-preserve-2018",
                                      "wildlife-preserve_inst_mdp__{:02d}.rddl".format(inst)),
    ) for inst, min_reward in zip(range(1,21), min_rewards)
]

# # Domains used in the IPC 2011.
IPC2011 = sorted(
    CROSSING_TRAFFIC_2011
    + ELEVATORS_2011
    + GAME_OF_LIFE_2011
    + NAVIGATION_2011
    + RECON_2011
    + SKILL_TEACHING_2011
    + SYSADMIN_2011
    + TRAFFIC_2011
)

# # Domains used in the IPC 2014.
IPC2014 = sorted(
    ACADEMIC_ADVISING_2014
    + CROSSING_TRAFFIC_2011
    + ELEVATORS_2011
    + SKILL_TEACHING_2011
    + TAMARISK_2014
    + TRAFFIC_2011
    + TRIANGLE_TIREWORLD_2014
    + WILDFIRE_2014
)

# # Domains used in the IPC 2018.
IPC2018 = sorted(
    ACADEMIC_ADVISING_2018
    + CHROMATIC_DICE_2018
    + COOPERATIVE_RECON_2018
    + EARTH_OBSERVATION_2018
    + MANUFACTURER_2018
    + PUSH_YOUR_LUCK_2018
    + RED_FINNED_BLUE_EYE_2018
    + WILDLIFE_PRESERVE_2018
)

# # Domains used in some IPC, removing duplicates.
IPC_UNTIL_2014 = sorted(list(set().union(IPC2011 + IPC2014)))
IPC_UNTIL_2018 = sorted(list(set().union(IPC2011 + IPC2014 + IPC2018)))
IPC_ALL = sorted(list(set().union(IPC2011 + IPC2014 + IPC2018)))

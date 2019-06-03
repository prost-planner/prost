#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""

Separate the domains according to their first IPC appearance.
All domains are in the directory "benchmarks/".

Domain are defined by their instances. Each instance object
contains the domain file, the problem file, the directory,
and the min- and max-scores.

Domains are listed alphabetically. IPC tracks are listed
chronologically.

(Note that some domains used in the IPC 2014 are originally
from the IPC 2011.)

"""

class Benchmark(object):
    def __init__(self, domain, problem, domain_path=None, min_score=None, max_score=None):
        self.domain = domain
        self.problem = problem
        self.path = domain_path
        self.min_score = min_score
        self.max_score = max_score

ACADEMIC_ADVISING_2014 = [
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__1.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__2.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__3.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__4.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__5.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__6.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__7.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__8.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__9.rddl", "academic-advising-2014", -200.0, None),
    Benchmark("academic_advising_mdp.rddl", "academic_advising_inst_mdp__10.rddl", "academic-advising-2014", -200.0, None),
]

ACADEMIC_ADVISING_2018 = [
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__01.rddl", "academic-advising-2018", -98.2, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__02.rddl", "academic-advising-2018", -89.5333333333, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__03.rddl", "academic-advising-2018", -100, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__04.rddl", "academic-advising-2018", -100, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__05.rddl", "academic-advising-2018", -100, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__06.rddl", "academic-advising-2018", -150, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__07.rddl", "academic-advising-2018", -150, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__08.rddl", "academic-advising-2018", -150, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__09.rddl", "academic-advising-2018", -150, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__10.rddl", "academic-advising-2018", -150, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__11.rddl", "academic-advising-2018", -200, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__12.rddl", "academic-advising-2018", -200, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__13.rddl", "academic-advising-2018", -200, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__14.rddl", "academic-advising-2018", -200, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__15.rddl", "academic-advising-2018", -200, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__16.rddl", "academic-advising-2018", -250, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__17.rddl", "academic-advising-2018", -250, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__18.rddl", "academic-advising-2018", -250, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__19.rddl", "academic-advising-2018", -250, None),
    Benchmark("academic-advising_mdp.rddl", "academic-advising_inst_mdp__20.rddl", "academic-advising-2018", -250, None),
]

CHROMATIC_DICE_2018 = [
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__01.rddl", "chromatic-dice-2018", 14.7066666667, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__02.rddl", "chromatic-dice-2018", 40.4933333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__03.rddl", "chromatic-dice-2018", 43.7733333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__04.rddl", "chromatic-dice-2018", 45.2666666667, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__05.rddl", "chromatic-dice-2018", 46.3333333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__06.rddl", "chromatic-dice-2018", 83.2533333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__07.rddl", "chromatic-dice-2018", 111.12, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__08.rddl", "chromatic-dice-2018", 34.1866666667, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__09.rddl", "chromatic-dice-2018", 67.8933333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__10.rddl", "chromatic-dice-2018", 80.9066666667, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__11.rddl", "chromatic-dice-2018", 34.3733333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__12.rddl", "chromatic-dice-2018", 75.2533333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__13.rddl", "chromatic-dice-2018", 34.0666666667, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__14.rddl", "chromatic-dice-2018", 40.3733333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__15.rddl", "chromatic-dice-2018", 70.3333333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__16.rddl", "chromatic-dice-2018", 173.653333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__17.rddl", "chromatic-dice-2018", 243.146666667, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__18.rddl", "chromatic-dice-2018", 130.253333333, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__19.rddl", "chromatic-dice-2018", 134.52, None),
    Benchmark("chromatic-dice_mdp.rddl", "chromatic-dice_inst_mdp__20.rddl", "chromatic-dice-2018", 136.653333333, None),
]

CROSSING_TRAFFIC_2011 = [
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__1.rddl", "crossing-traffic-2011", -32.73, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__2.rddl", "crossing-traffic-2011", -34.27, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__3.rddl", "crossing-traffic-2011", -39.03, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__4.rddl", "crossing-traffic-2011", -40.0, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__5.rddl", "crossing-traffic-2011", -38.97, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__6.rddl", "crossing-traffic-2011", -40.0, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__7.rddl", "crossing-traffic-2011", -39.47, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__8.rddl", "crossing-traffic-2011", -40.0, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__9.rddl", "crossing-traffic-2011", -39.17, None),
    Benchmark("crossing_traffic_mdp.rddl", "crossing_traffic_inst_mdp__10.rddl", "crossing-traffic-2011", -40.0, None),
]


COOPERATIVE_RECON_2018 = [
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__01.rddl", "cooperative-recon-2018", 0.179066666667, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__02.rddl", "cooperative-recon-2018", 0.310533333333, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__03.rddl", "cooperative-recon-2018", 0.0904, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__04.rddl", "cooperative-recon-2018", 0.5424, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__05.rddl", "cooperative-recon-2018", 0.187466666667, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__06.rddl", "cooperative-recon-2018", 0.064, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__07.rddl", "cooperative-recon-2018", 0.330533333333, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__08.rddl", "cooperative-recon-2018", 0.0, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__09.rddl", "cooperative-recon-2018", 0.0, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__10.rddl", "cooperative-recon-2018", 0.502933333333, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__11.rddl", "cooperative-recon-2018", 0.0864, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__12.rddl", "cooperative-recon-2018", 0.0, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__13.rddl", "cooperative-recon-2018", 0.154133333333, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__14.rddl", "cooperative-recon-2018", 0.128933333333, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__15.rddl", "cooperative-recon-2018", 0.0, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__16.rddl", "cooperative-recon-2018", 0.0113333333333, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__17.rddl", "cooperative-recon-2018", 0.139733333333, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__18.rddl", "cooperative-recon-2018", 0.0, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__19.rddl", "cooperative-recon-2018", 0.0, None),
    Benchmark("cooperative-recon_mdp.rddl", "cooperative-recon_inst_mdp__20.rddl", "cooperative-recon-2018", 0.0, None),
]

EARTH_OBSERVATION_2018 = [
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__01.rddl", "earth-observation-2018", -46.0533333333, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__02.rddl", "earth-observation-2018", -834.24, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__03.rddl", "earth-observation-2018", -1048.32, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__04.rddl", "earth-observation-2018", -2107.33333333, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__05.rddl", "earth-observation-2018", -1246.04, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__06.rddl", "earth-observation-2018", -402.013333333, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__07.rddl", "earth-observation-2018", -137.12, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__08.rddl", "earth-observation-2018", -629.106666667, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__09.rddl", "earth-observation-2018", -1751.53333333, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__10.rddl", "earth-observation-2018", -4145.85333333, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__11.rddl", "earth-observation-2018", -1152.6, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__12.rddl", "earth-observation-2018", -2394.82666667, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__13.rddl", "earth-observation-2018", -2473.06666667, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__14.rddl", "earth-observation-2018", -11515.16, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__15.rddl", "earth-observation-2018", -3343.22666667, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__16.rddl", "earth-observation-2018", -621.04, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__17.rddl", "earth-observation-2018", -2199.96, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__18.rddl", "earth-observation-2018", -3611.38666667, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__19.rddl", "earth-observation-2018", -5621.30666667, None),
    Benchmark("earth-observation_mdp.rddl", "earth-observation_inst_mdp__20.rddl", "earth-observation-2018", -14684.8, None),
]

ELEVATORS_2011 = [
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__1.rddl", "elevators-2011", -65.2, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__2.rddl", "elevators-2011", -54.7, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__3.rddl", "elevators-2011", -71.7, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__4.rddl", "elevators-2011", -94.07, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__5.rddl", "elevators-2011", -111.8, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__6.rddl", "elevators-2011", -118.67, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__7.rddl", "elevators-2011", -129.63, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__8.rddl", "elevators-2011", -146.9, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__9.rddl", "elevators-2011", -160.37, None),
    Benchmark("elevators_mdp.rddl", "elevators_inst_mdp__10.rddl", "elevators-2011", -109.7, None),
]

GAME_OF_LIFE_2011 = [
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__1.rddl", "game-of-life-2011", 70.53, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__2.rddl", "game-of-life-2011", 73.27, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__3.rddl", "game-of-life-2011", 107.17, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__4.rddl", "game-of-life-2011", 167.23, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__5.rddl", "game-of-life-2011", 203.07, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__6.rddl", "game-of-life-2011", 224.17, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__7.rddl", "game-of-life-2011", 264.97, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__8.rddl", "game-of-life-2011", 295.23, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__9.rddl", "game-of-life-2011", 311.47, None),
    Benchmark("game_of_life_mdp.rddl", "game_of_life_inst_mdp__10.rddl", "game-of-life-2011", 176.73, None),
]


MANUFACTURER_2018 = [
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__01.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__02.rddl", "manufacturer-2018", 120.510933333, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__03.rddl", "manufacturer-2018", 178.050686667, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__04.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__05.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__06.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__07.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__08.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__09.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__10.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__11.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__12.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__13.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__14.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__15.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__16.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__17.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__18.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__19.rddl", "manufacturer-2018", 0.0, None),
    Benchmark("manufacturer_mdp.rddl", "manufacturer_inst_mdp__20.rddl", "manufacturer-2018", 0.0, None),
]

NAVIGATION_2011 = [
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__1.rddl", "navigation-2011", -37.27, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__2.rddl", "navigation-2011", -40.0, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__3.rddl", "navigation-2011", -40.0, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__4.rddl", "navigation-2011", -39.83, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__5.rddl", "navigation-2011", -40.0, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__6.rddl", "navigation-2011", -40.0, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__7.rddl", "navigation-2011", -40.0, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__8.rddl", "navigation-2011", -38.73, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__9.rddl", "navigation-2011", -40.0, None),
    Benchmark("navigation_mdp.rddl", "navigation_inst_mdp__10.rddl", "navigation-2011", -40.0, None),
]


PUSH_YOUR_LUCK_2018 = [
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__01.rddl", "push-your-luck-2018", 31.1466666667, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__02.rddl", "push-your-luck-2018", 37.5466666667, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__03.rddl", "push-your-luck-2018", 48.5333333333, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__04.rddl", "push-your-luck-2018", 21.6016666667, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__05.rddl", "push-your-luck-2018", 21.300234375, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__06.rddl", "push-your-luck-2018", 175.707434896, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__07.rddl", "push-your-luck-2018", 17.4816666667, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__08.rddl", "push-your-luck-2018", 27.7428255208, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__09.rddl", "push-your-luck-2018", 2162.97210124, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__10.rddl", "push-your-luck-2018", 10.1816666667, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__11.rddl", "push-your-luck-2018", 13.45796875, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__12.rddl", "push-your-luck-2018", 1599.48311483, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__13.rddl", "push-your-luck-2018", 8.67, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__14.rddl", "push-your-luck-2018", 11.2679557292, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__15.rddl", "push-your-luck-2018", 77.8681770833, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__16.rddl", "push-your-luck-2018", 50.9986979167, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__17.rddl", "push-your-luck-2018", 11.5055013021, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__18.rddl", "push-your-luck-2018", 6.939375, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__19.rddl", "push-your-luck-2018", 2.03479166667, None),
    Benchmark("push-your-luck_mdp.rddl", "push-your-luck_inst_mdp__20.rddl", "push-your-luck-2018", 2.19208333333, None),
]

RECON_2011 = [
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__1.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__2.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__3.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__4.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__5.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__6.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__7.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__8.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__9.rddl", "recon-2011", 0.0, None),
    Benchmark("recon_mdp.rddl", "recon_inst_mdp__10.rddl", "recon-2011", 0.0, None),
]

RED_FINNED_BLUE_EYES_2018 = [
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__01.rddl", "red-finned-blue-eyes-2018", -3233.33333, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__02.rddl", "red-finned-blue-eyes-2018", 542.8, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__03.rddl", "red-finned-blue-eyes-2018", -6624.4, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__04.rddl", "red-finned-blue-eyes-2018", -5152.0, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__05.rddl", "red-finned-blue-eyes-2018", -4486.26666667, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__06.rddl", "red-finned-blue-eyes-2018", -9000.66666667, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__07.rddl", "red-finned-blue-eyes-2018", 11796.1333333, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__08.rddl", "red-finned-blue-eyes-2018", -7258.8, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__09.rddl", "red-finned-blue-eyes-2018", -7496.13333333, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__10.rddl", "red-finned-blue-eyes-2018", 3910.93333333, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__11.rddl", "red-finned-blue-eyes-2018", -481.866666667, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__12.rddl", "red-finned-blue-eyes-2018", 1886.0, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__13.rddl", "red-finned-blue-eyes-2018", -6857.86666667, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__14.rddl", "red-finned-blue-eyes-2018", 4011.86666667, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__15.rddl", "red-finned-blue-eyes-2018", -8514.66666667, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__16.rddl", "red-finned-blue-eyes-2018", 18367.6, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__17.rddl", "red-finned-blue-eyes-2018", 8665.6, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__18.rddl", "red-finned-blue-eyes-2018", -11896.6666667, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__19.rddl", "red-finned-blue-eyes-2018", -6029.46666667, None),
    Benchmark("red-finned-blue-eyes_mdp.rddl", "red-finned-blue-eyes_inst_mdp__20.rddl", "red-finned-blue-eyes-2018", -1241.86666667, None),
]

SKILL_TEACHING_2011 = [
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__1.rddl", "skill-teaching-2011", 39.34, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__2.rddl", "skill-teaching-2011", 33.75, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__3.rddl", "skill-teaching-2011", -83.23, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__4.rddl", "skill-teaching-2011", -108.73, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__5.rddl", "skill-teaching-2011", -285.95, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__6.rddl", "skill-teaching-2011", -302.99, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__7.rddl", "skill-teaching-2011", -382.86, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__8.rddl", "skill-teaching-2011", -534.7, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__9.rddl", "skill-teaching-2011", -540.62, None),
    Benchmark("skill_teaching_mdp.rddl", "skill_teaching_inst_mdp__10.rddl", "skill-teaching-2011", -632.0, None),
]

SYSADMIN_2011 = [
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__1.rddl", "sysadmin-2011", 210.18, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__2.rddl", "sysadmin-2011", 166.68, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__3.rddl", "sysadmin-2011", 345.4, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__4.rddl", "sysadmin-2011", 311.79, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__5.rddl", "sysadmin-2011", 443.05, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__6.rddl", "sysadmin-2011", 407.16, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__7.rddl", "sysadmin-2011", 513.74, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__8.rddl", "sysadmin-2011", 426.46, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__9.rddl", "sysadmin-2011", 612.06, None),
    Benchmark("sysadmin_mdp.rddl", "sysadmin_inst_mdp__10.rddl", "sysadmin-2011", 471.1, None),
]

TAMARISK_2014 = [
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__1.rddl", "tamarisk-2014", -633.01, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__2.rddl", "tamarisk-2014", -948.98, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__3.rddl", "tamarisk-2014", -820.54, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__4.rddl", "tamarisk-2014", -1178.76, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__5.rddl", "tamarisk-2014", -1276.74, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__6.rddl", "tamarisk-2014", -1432.27, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__7.rddl", "tamarisk-2014", -1452.65, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__8.rddl", "tamarisk-2014", -1629.29, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__9.rddl", "tamarisk-2014", -1463.72, None),
    Benchmark("tamarisk_mdp.rddl", "tamarisk_inst_mdp__10.rddl", "tamarisk-2014", -1758.88, None),
]

TRAFFIC_2011 = [
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__1.rddl", "traffic-2011", -19.97, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__2.rddl", "traffic-2011", -47.87, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__3.rddl", "traffic-2011", -63.93, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__4.rddl", "traffic-2011", -119.33, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__5.rddl", "traffic-2011", -120.7, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__6.rddl", "traffic-2011", -156.67, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__7.rddl", "traffic-2011", -146.57, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__8.rddl", "traffic-2011", -154.37, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__9.rddl", "traffic-2011", -108.03, None),
    Benchmark("traffic_mdp.rddl", "traffic_inst_mdp__10.rddl", "traffic-2011", -243.7, None),
]

TRIANGLE_TIREWORLD_2014 = [
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__1.rddl", "triangle-tireworld-2014", -32.27, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__2.rddl", "triangle-tireworld-2014", -28.47, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__3.rddl", "triangle-tireworld-2014", -40.0, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__4.rddl", "triangle-tireworld-2014", -40.0, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__5.rddl", "triangle-tireworld-2014", -40.0, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__6.rddl", "triangle-tireworld-2014", -40.0, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__7.rddl", "triangle-tireworld-2014", -40.0, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__8.rddl", "triangle-tireworld-2014", -40.0, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__9.rddl", "triangle-tireworld-2014", -40.0, None),
    Benchmark("triangle-tireworld_mdp.rddl", "triangle-tireworld_inst_mdp__10.rddl", "triangle-tireworld-2014", -40.0, None),
]

WILDFIRE_2014 = [
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__1.rddl", "wildfire-2014", -4126.83, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__2.rddl", "wildfire-2014", -14146.83, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__3.rddl", "wildfire-2014", -10424.5, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__4.rddl", "wildfire-2014", -23154.0, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__5.rddl", "wildfire-2014", -8169.83, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__6.rddl", "wildfire-2014", -24726.33, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__7.rddl", "wildfire-2014", -16646.67, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__8.rddl", "wildfire-2014", -26196.67, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__9.rddl", "wildfire-2014", -18122.5, None),
    Benchmark("wildfire_mdp.rddl", "wildfire_inst_mdp__10.rddl", "wildfire-2014", -27999.67, None),
]

WILDLIFE_PRESERVE_2018 = [
    Benchmark("wildlife-preserve_01_mdp.rddl", "wildlife-preserve_inst_mdp__01.rddl", "wildlife-preserve-2018", 861.6948, None),
    Benchmark("wildlife-preserve_02_mdp.rddl", "wildlife-preserve_inst_mdp__02.rddl", "wildlife-preserve-2018", 772.2744, None),
    Benchmark("wildlife-preserve_03_mdp.rddl", "wildlife-preserve_inst_mdp__03.rddl", "wildlife-preserve-2018", 784.024133333, None),
    Benchmark("wildlife-preserve_04_mdp.rddl", "wildlife-preserve_inst_mdp__04.rddl", "wildlife-preserve-2018", 661.932, None),
    Benchmark("wildlife-preserve_05_mdp.rddl", "wildlife-preserve_inst_mdp__05.rddl", "wildlife-preserve-2018", 537.096, None),
    Benchmark("wildlife-preserve_06_mdp.rddl", "wildlife-preserve_inst_mdp__06.rddl", "wildlife-preserve-2018", 701.793866667, None),
    Benchmark("wildlife-preserve_07_mdp.rddl", "wildlife-preserve_inst_mdp__07.rddl", "wildlife-preserve-2018", 268.610666667, None),
    Benchmark("wildlife-preserve_08_mdp.rddl", "wildlife-preserve_inst_mdp__08.rddl", "wildlife-preserve-2018", 727.618, None),
    Benchmark("wildlife-preserve_09_mdp.rddl", "wildlife-preserve_inst_mdp__09.rddl", "wildlife-preserve-2018", 751.386666667, None),
    Benchmark("wildlife-preserve_10_mdp.rddl", "wildlife-preserve_inst_mdp__10.rddl", "wildlife-preserve-2018", 636.292266667, None),
    Benchmark("wildlife-preserve_11_mdp.rddl", "wildlife-preserve_inst_mdp__11.rddl", "wildlife-preserve-2018", 1049.56306667, None),
    Benchmark("wildlife-preserve_12_mdp.rddl", "wildlife-preserve_inst_mdp__12.rddl", "wildlife-preserve-2018", 574.419066667, None),
    Benchmark("wildlife-preserve_13_mdp.rddl", "wildlife-preserve_inst_mdp__13.rddl", "wildlife-preserve-2018", 758.8136, None),
    Benchmark("wildlife-preserve_14_mdp.rddl", "wildlife-preserve_inst_mdp__14.rddl", "wildlife-preserve-2018", 777.9772, None),
    Benchmark("wildlife-preserve_15_mdp.rddl", "wildlife-preserve_inst_mdp__15.rddl", "wildlife-preserve-2018", 1252.9972, None),
    Benchmark("wildlife-preserve_16_mdp.rddl", "wildlife-preserve_inst_mdp__16.rddl", "wildlife-preserve-2018", 695.311866667, None),
    Benchmark("wildlife-preserve_17_mdp.rddl", "wildlife-preserve_inst_mdp__17.rddl", "wildlife-preserve-2018", 1467.67613333, None),
    Benchmark("wildlife-preserve_18_mdp.rddl", "wildlife-preserve_inst_mdp__18.rddl", "wildlife-preserve-2018", 547.9844, None),
    Benchmark("wildlife-preserve_19_mdp.rddl", "wildlife-preserve_inst_mdp__19.rddl", "wildlife-preserve-2018", 1595.51253333, None),
    Benchmark("wildlife-preserve_20_mdp.rddl", "wildlife-preserve_inst_mdp__20.rddl", "wildlife-preserve-2018", 769.4372, None),
]


# Domains used in the IPC 2011.
IPC2011 =  CROSSING_TRAFFIC_2011 + \
            ELEVATORS_2011 + \
            GAME_OF_LIFE_2011 + \
            NAVIGATION_2011 + \
            RECON_2011 +\
            SKILL_TEACHING_2011 + \
            SYSADMIN_2011 + \
            TRAFFIC_2011

# Domains used in the IPC 2014.
IPC2014 =  ACADEMIC_ADVISING_2014 + \
            CROSSING_TRAFFIC_2011 + \
            ELEVATORS_2011 + \
            SKILL_TEACHING_2011 + \
            TAMARISK_2014 + \
            TRAFFIC_2011 + \
            TRIANGLE_TIREWORLD_2014 + \
            WILDFIRE_2014

# Domains used in the IPC 2018.
IPC2018 = ACADEMIC_ADVISING_2018 + \
           CHROMATIC_DICE_2018 +  \
           COOPERATIVE_RECON_2018 + \
           EARTH_OBSERVATION_2018 + \
           MANUFACTURER_2018 + \
           PUSH_YOUR_LUCK_2018 + \
           RED_FINNED_BLUE_EYES_2018 + \
           WILDLIFE_PRESERVE_2018

# Domains used in some IPC, removing duplicates.
IPC_ALL = list(set().union(IPC2011 + IPC2014 + IPC2018))

#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""

Separate the domains according to their first IPPC appearance.
All domains are in the directory "benchmarks/".

"""

IPPC2011 = ["crossing-traffic2011",
            "elevators2011",
            "game-of-life2011",
            "navigation2011",
            "recon2011",
            "skill-teaching2011",
            "sysadmin2011",
            "traffic2011"]

IPPC2014 = ["academic-advising2014",
            "tamarisk2014",
            "triangle-tireworld2014",
            "wildfire2014",
            "crossing-traffic2011",
            "elevators2011",
            "skill-teaching2011",
            "traffic2011"]

IPPC2018 = ["academic-advising-2018",
            "chromatic-dice-2018",
            "cooperative-recon-2018",
            "earth-observation-2018",
            "manufacturer-2018",
            "push-your-luck-2018",
            "red-finned-blue-eye-2018",
            "wildlife-preserve-2018"]

IPPC_ALL = IPPC2011 + IPPC2014 + IPPC2018

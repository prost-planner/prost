#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""

Separate the domains according to their first IPPC appearance.
All domains are in the directory "benchmarks/".

(Note that some domains used in the IPPC 2014 are originally
from the IPPC 2011.)

"""

# Domains used in the IPPC 2011.
IPPC2011 = ["crossing-traffic-2011",
            "elevators-2011",
            "game-of-life-2011",
            "navigation-2011",
            "recon-2011",
            "skill-teaching-2011",
            "sysadmin-2011",
            "traffic-2011"]

# Domains used in the IPPC 2014.
IPPC2014 = ["academic-advising-2014",
            "tamarisk-2014",
            "triangle-tireworld-2014",
            "wildfire-2014",
            "crossing-traffic-2011",
            "elevators-2011",
            "skill-teaching-2011",
            "traffic-2011"]

# Domains used in the IPPC 2018.
IPPC2018 = ["academic-advising-2018",
            "chromatic-dice-2018",
            "cooperative-recon-2018",
            "earth-observation-2018",
            "manufacturer-2018",
            "push-your-luck-2018",
            "red-finned-blue-eye-2018",
            "wildlife-preserve-2018"]

# Domains used in some IPPC, removing duplicates.
IPPC_ALL = list(set().union(IPPC2011 + IPPC2014 + IPPC2018))

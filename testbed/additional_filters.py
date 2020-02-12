"""
File with aditional filters for the Prost reports using Lab.

All filters must receive two runs as input.
"""


def domain_as_category(run1, run2):
    """Compare two runs of the same problem.

    NOTE: run2['domain'] has the same value, because we always

    """
    return run1["domain"]


def time_improvement(run1, run2):
    """Compare and order two runs based on their time improvement.

    """
    time1 = run1.get("time", 4000)
    time2 = run2.get("time", 4000)
    if time1 > time2:
        return "better"
    if time1 == time2:
        return "equal"
    return "worse"

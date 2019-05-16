"""
File with aditional filters for the Prost reports using Lab.
"""


def domain_as_category(run1, run2):
    # run2['domain'] has the same value, because we always
    # compare two runs of the same problem.
    return run1['domain']

def improvement(run1, run2):
    time1 = run1.get('time', 4000)
    time2 = run2.get('time', 4000)
    if time1 > time2:
        return 'better'
    if time1 == time2:
        return 'equal'
    return 'worse'

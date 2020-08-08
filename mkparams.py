#!/usr/bin/python

import re
import sys

from lifelib.genera import rule_property, genus_list
from lifelib.autocompile import set_rules, sanirule
from lifelib.pythlib.samples import validate_symmetry


def main(argc, argv):

    if argc < 2:
        rulestring = 'b3s23'
    else:
        rulestring = argv[1]

    # Convert rulestrings such as 'B3/S23' into 'b3s23':
    newrule = sanirule(rulestring)
    if newrule != rulestring:
        print("Warning: \033[1;31m" + rulestring + "\033[0m interpreted as \033[1;32m" + newrule + "\033[0m")
        rulestring = newrule

    # Check isotropic:
    g = [x for x in genus_list if x['fullname'] == 'isotropic']
    assert(len(g) == 1)
    g = g[0]
    if not re.match(g['regex'], rulestring):
        raise ValueError("Error: rule %s is not an isotropic 2-state Moore-neighbourhood rule" % rulestring)

    # Generate code:
    set_rules(rulestring)


if __name__ == '__main__':

    main(len(sys.argv), sys.argv)

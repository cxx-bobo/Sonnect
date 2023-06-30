import sys
import glob

'''
argv[1] - project:      project name (abbreviation)
'''
kArgvIndex_project = 1

# add all local include files
include_dirs = [
    "include",
    "include/apps",
    "include/{}_doca_utils".format(sys.argv[kArgvIndex_project]),
    "include/{}_utils".format(sys.argv[kArgvIndex_project])
]

for i in include_dirs:
    print(i)
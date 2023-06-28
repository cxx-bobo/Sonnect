import sys
import glob

'''
argv[1] - app:          application name
'''
kArgvIndex_app = 1

# add all local source files
sources = glob.glob("./src/*.c") + glob.glob("./src/sc_utils/*.c") + glob.glob("./src/apps/sc_{}/*.c".format(sys.argv[kArgvIndex_app])) + glob.glob("./src/*.cpp") + glob.glob("./src/sc_utils/*.cpp") + glob.glob("./src/apps/sc_{}/*.cpp".format(sys.argv[kArgvIndex_app]))

for i in sources:
    print(i)
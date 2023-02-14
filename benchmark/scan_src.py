import sys
import glob

sources = glob.glob("./src/*.c") + glob.glob("./src/sc_{}/*.c".format(sys.argv[1]))
for i in sources:
    print(i)
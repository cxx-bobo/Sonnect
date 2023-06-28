import sys
import glob

'''
argv[1] - app:          application name
'''
kArgvIndex_app = 1

# add all local source files
sources = glob.glob("./src/**/*.c", recursive=True) + glob.glob("./src/**/*.cpp", recursive=True) 

# exclude all app-related code
sources = list(set(sources) - set(glob.glob("./src/apps/**/*.c", recursive=True)))
sources = list(set(sources) - set(glob.glob("./src/apps/**/*.cpp", recursive=True)))

# join back the code of the specify app
sources += glob.glob("./src/apps/sc_{}/**/*.c".format(sys.argv[kArgvIndex_app]), recursive=True)
sources += glob.glob("./src/apps/sc_{}/**/*.cpp".format(sys.argv[kArgvIndex_app]), recursive=True)
    
for i in sources:
    print(i)
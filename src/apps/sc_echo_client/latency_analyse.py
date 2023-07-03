import numpy as np
import sys

data = np.loadtxt(sys.argv[1])
lat = data[:,1]
print("%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s" % ("Average", "5%", "25%", "50%", "99%", "99.9%", "Max."))
print("%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f" % (np.average(lat), np.percentile(lat, 5), np.percentile(lat, 25), np.percentile(lat, 50), np.percentile(lat, 99), np.percentile(lat, 99.9), np.amax(lat)))

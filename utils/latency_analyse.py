import numpy as np
import sys

data = np.loadtxt(sys.argv[1])

core = t1s = data[:,0]
t1s = data[:,1]
t2s = data[:,2]

latency = t2s - t1s

for i in range(0, len(latency)):
    if t2s[i] < t1s[i]:
        print(f"{core[i]} {t2s[i]}, {t1s[i]}")

print("%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s" % ("Average", "5%", "25%", "50%", "99%", "99.9%", "Max."))
print("%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f" % (
    np.mean(latency),
    np.percentile(latency, 5),
    np.percentile(latency, 25),
    np.percentile(latency, 50),
    np.percentile(latency, 99),
    np.percentile(latency, 99.9),
    np.amax(latency)
))

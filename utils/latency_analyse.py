import numpy as np
import sys

data = np.loadtxt(sys.argv[1])

client_send_ts = data[:,1]
server_recv_ts = data[:,2]
server_send_ts = data[:,3]
client_recv_ts = data[:,4]

client_latency = client_recv_ts - client_send_ts
server_latency = server_send_ts - server_recv_ts

for i in range(0, len(client_recv_ts)):
    if client_recv_ts[i] < client_send_ts[i]:
        print(f"{client_recv_ts[i]}, {client_send_ts[i]}")

print("%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s" % ("Average", "5%", "25%", "50%", "99%", "99.9%", "Max."))
print("%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f" % (
    np.mean(client_latency),
    np.percentile(client_latency, 5),
    np.percentile(client_latency, 25),
    np.percentile(client_latency, 50),
    np.percentile(client_latency, 99),
    np.percentile(client_latency, 99.9),
    np.amax(client_latency)
))

print("%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f" % (
    np.mean(server_latency),
    np.percentile(server_latency, 5),
    np.percentile(server_latency, 25),
    np.percentile(server_latency, 50),
    np.percentile(server_latency, 99),
    np.percentile(server_latency, 99.9), 
    np.amax(server_latency)
))

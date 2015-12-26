import csv
import sys
import matplotlib.pyplot as plt
import numpy as np
 
# for P lines
#2-> str,
#3 -> clock_time(),
#4-> P,
#5->rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
#6-> seqno,
#7 -> all_cpu,
#8-> all_lpm,
#9-> all_transmit,
#10-> all_listen,
#11-> all_idle_transmit,
#12-> all_idle_listen,
#13->cpu,
#14-> lpm,
#15-> transmit,
#16-> listen,
#17 ->idle_transmit,
#18 -> idle_listen, [RADIO STATISTICS...]
 
from collections import defaultdict
nodeOverTime =  defaultdict(lambda: defaultdict(list))

battery_energy = 16000.
hours_per_day = 24.
seconds_per_hour = 60. * 60.
seconds_per_day = seconds_per_hour * hours_per_day

ticks_per_second = 32768.
ticks_per_hour = ticks_per_second * seconds_per_hour
ticks_per_day = ticks_per_second * seconds_per_day

with open(sys.argv[1], 'rb') as f:
    reader = csv.reader(f,delimiter=' ')
    for row in reader:
#        print row
        if row[4] is not 'P':
            continue

        for i in [7, 8, 9, 10]:
            nodeOverTime[row[5]][i].append(int(row[i]) / ticks_per_second)

size = np.ravel([[
        len(nodeOverTime[key][i]) for i in [7, 8, 9, 10]
    ] for key in nodeOverTime.keys()]).min()
        
power = np.array([5.4, 0.0153, 58.5, 65.4]) / 1000
usage = np.array([[
        nodeOverTime[key][7][:size],
        nodeOverTime[key][8][:size],
        nodeOverTime[key][9][:size],
        nodeOverTime[key][10][:size],
    ] for key in nodeOverTime.keys()]).swapaxes(0, 2).swapaxes(1, 2)

energy = np.sum(power * usage, axis=2) / seconds_per_hour
time = np.sum(usage, axis=2) / seconds_per_hour

#print energy.shape, time.shape

x = np.max(time, axis=1)
y = np.max(energy, axis=1)
#print x.shape, y.shape

A = np.vstack([x, np.ones(x.shape)]).T
m, c = np.linalg.lstsq(A, y)[0]
print "Dias:", 16000. / m / seconds_per_day



plt.plot(time, energy)
plt.plot(x, m * x + c, 'o', label='Fitted line')
plt.plot(np.mean(time, axis=1), np.mean(energy, axis=1), ".")
plt.plot(np.percentile(time, axis=1, q=50), np.percentile(energy, axis=1, q=50), "x")






























































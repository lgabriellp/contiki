import sys
import glob
import pandas as pd
import numpy as np
import scipy.stats as st
import matplotlib.pyplot as plt

battery_energy = 16000.
hours_per_day = 24.
ticks_per_second = 32768.
seconds_per_day = 60. * 60 * hours_per_day
seconds_per_year = seconds_per_day * 365.
powertrace_period = 60.

def analyse_lifetime(path):
    trace = pd.read_csv(path,
        comment="#",
        sep=" ",
        usecols=[1, 3, 5, 6, 7, 8],
#        usecols=[3, 5, 13, 14, 15, 16],
        names=["time", "node", "cpu", "lpm", "transmit", "listen"])

    #trace.time = np.round(trace.time / 1000)
    #trace.time /= powertrace_period

    # time(s) = powertrace_time(ticks in a period) / powertrace_period(s) / rtimer_second(ticks)
    #trace[["cpu", "lpm", "transmit", "listen"]] /= powertrace_period * ticks_per_second
    trace[["cpu", "lpm", "transmit", "listen"]] /= ticks_per_second

    # power(W) = power(mW) / 1000
    power = 10**-3 * np.array([5.4, 0.0153, 58.5, 65.4])

    #energy(Joules) = power(W) * time(s)
    energy_per_node = (power * trace.set_index(["time", "node"])).sum(axis=1).unstack()
    del energy_per_node[1]

    energy_per_node.plot()
    plt.show()

    energy_max = energy_per_node.max(axis=1)
    x = energy_max.index
    y = energy_max

    #print energy_history_max

    A = np.vstack([x, np.ones(x.shape)]).T
    m, c = np.linalg.lstsq(A, y)[0]

    return battery_energy / m / seconds_per_day


paths = glob.glob("*p.csv")
lifetime = np.array([analyse_lifetime(path) for path in paths])

print lifetime

mean = lifetime.mean()
sem = st.sem(lifetime)
lower, upper = st.t.interval(.99, len(lifetime), mean, sem)
print mean, lower, upper, (upper - lower) / mean, (upper - lower) / mean < 0.1, 



"""

history = trace.pivot("node").stack().dropna()
energy_history = (power * history).sum(axis=0).unstack()
del energy_history[1] # node 1 has infinite energy
energy_history_per_node = energy_history.dropna()
energy_history_per_node.plot()
plt.show()


print energy_history_per_node.stack().max(level=0)

"""


"""
#energy_history_per_node_intervals = st.norm.interval(.95, energy_history_per_node.mean(axis=1), energy_history_per_node.std(axis=1))
#plt.fill_between(xrange(len(energy_history_per_node_intervals[0])), energy_history_per_node_intervals[0], energy_history_per_node_intervals[1])

#plt.plot(energy_history_per_node.index, energy_history_per_node_intervals)

"""


#energy = (power * times)
#energy.plot()
#plt.show()

#energy = (power * times).sum(level=0).sum(axis=1)
#elapsed_seconds = times.sum(level=0).sum(axis=1)
#duration_per_year = battery_energy / (energy / elapsed_seconds) / seconds_per_day


#print duration_per_year.mean(), duration_per_year.quantile(.95), duration_per_year.std()

"""
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

energy = (power * usage).sum(axis=2) / seconds_per_hour
time = usage.sum(axis=2) / seconds_per_hour

#print energy.shape, time.shape

x = time.max(axis=1)
y = energy.max(axis=1)
#print x.shape, y.shape

A = np.vstack([x, np.ones(x.shape)]).T
m, c = np.linalg.lstsq(A, y)[0]
print "A:", m, "b:", c
print "Dias:", 16000. / m / seconds_per_day

plt.plot(time, energy)
plt.plot(x, m * x + c, 'o', label='Fitted line')
plt.plot(time.mean(axis=1), energy.mean(axis=1), ".")
plt.plot(np.percentile(time, axis=1, q=50), np.percentile(energy, axis=1, q=50), "x")
plt.legend()
plt.show()
"""

# coding=utf-8
import re
import gc
import sys
import glob
import numpy as np
import pandas as pd
import scipy.stats as st
import matplotlib.pyplot as plt
import sklearn.linear_model
from collections import defaultdict

battery_energy = 16000.
hours_per_day = 24.
ticks_per_second = 32768.
seconds_per_day = 60. * 60 * hours_per_day
seconds_per_year = seconds_per_day * 365.
powertrace_period = 60.

    
def round_lifetime(path):
    trace = pd.read_csv(path,
        comment="#",
        sep=" ",
        usecols=[1, 3, 5, 6, 7, 8],
        names=["time", "node", "cpu", "lpm", "transmit", "listen"])

    # power(W) = power(mW) / 1000
    power = 10**-3 * np.array([5.4, 0.0153, 58.5, 65.4])
    
    # usage(s) = usage(ticks) / ticks_per_second
    trace[["cpu", "lpm", "transmit", "listen"]] /= ticks_per_second

    #energy(J) = SUM(power(W) * usage(s), power_profile)
    trace["energy"] = (power * trace[["cpu", "lpm", "transmit", "listen"]]).sum(axis=1)

    energy = trace[trace.node != 1].groupby(["time"]).max().energy

    model = sklearn.linear_model.LinearRegression()
    model.fit(energy.index.reshape(-1, 1), energy.values.reshape(-1, 1))

    return (battery_energy / model.coef_ / seconds_per_day)[0][0]

def lifetime(path):
    settings = defaultdict(list)
    results = []

    for trace_path in glob.glob(path + "/**/[!a-z]*p.csv"):
        res = re.search(r".*/(\d+)/(\d+)p.csv$", trace_path)
        setting = res.group(1)
        game = res.group(2)
        print trace_path, setting, game
        settings[setting].append(round_lifetime(trace_path))


    for setting, samples in settings.items():
        print setting, samples
        samples = np.array(samples)
        mean = samples.mean()
        sem = st.sem(samples)
        rv = len(samples) -1
        lower, upper = st.t.interval(.99, rv, mean, sem)
        yerr = (upper - lower) / 2
#        print "Setting(%s): %f %d %d" % (game, mean, rv, (yerr/mean) < 0.1), samples
        results.append(dict(name=setting, mean=mean, yerr=yerr))

    results.sort(key=lambda x: int(x["name"]))
    return results


def net(path):
    trace = pd.DataFrame()

    for trace_path in glob.glob(path + "/**/[!a-z]*n.csv"):
        gc.collect()
        res = re.search(r".*/(\d+)/(\d+)n.csv$", trace_path)
        setting = res.group(1)
        game = res.group(2)
        print trace_path
        game_trace = pd.read_csv(trace_path,
                    comment="#",
                    sep=" ",
                    usecols=range(1, 3) + range(9, 12),
                    names=[
                        "time",
                        "node",
#                        "sent",
#                        "recv",
#                        "timedout",
#                        "map",
#                        "emit",
#                        "reduce",
                        "ram",
                        "normal",
                        "urgent"])
        game_trace["game"] = int(game)
        game_trace["setting"] = int(setting)
        trace = trace.append(game_trace, ignore_index=True)
        print trace.shape


    return trace.groupby("setting").quantile(.99)[["ram", "normal", "urgent"]]

path = sys.argv[1] if len(sys.argv) > 1 else "."
lt = lifetime(path)
instant = net(path)
x = instant.index / 2 + 0.5
plt.suptitle(u"Evolução do Número de Aplicações")
plt.figure(1)

ax = plt.subplot(311)
ax.set_xlim((-.5, len(lt)))
ax.yaxis.grid(True)
plt.ylabel("Tempo de Vida (dias)")
plt.xticks(np.arange(len(lt)), map(lambda x: x["name"], lt))
plt.errorbar(np.arange(len(lt)), map(lambda x: x["mean"], lt), map(lambda x: x["yerr"], lt), color="g")

ax = plt.subplot(312)
ax.bar(x, instant.ram, align="center")
plt.ylabel(u"Memória Dinâmica (bytes)")
plt.xticks(x, map(int, instant.index))

ax = plt.subplot(313)
ax.bar(x, instant[["normal", "urgent"]], color="g", align="center")
plt.ylabel(u"Pares Reduzidos")
plt.xticks(x, map(int, instant.index))

plt.tight_layout()
plt.show()


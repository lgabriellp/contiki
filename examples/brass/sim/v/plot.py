# coding: utf-8
from collections import defaultdict
import mpl_toolkits.mplot3d as plt3d
import sklearn.linear_model
import matplotlib.pyplot as plt
import scipy.stats as st
import pandas as pd
import numpy as np
import glob
import re

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

    for trace_path in glob.glob(path + "/**/*.csv"):
        res = re.search(r".*/(\d+)-(\d+)/(\d+)p.csv$", trace_path)
        if not res: continue
        
        setting = (int(res.group(1)), int(res.group(2)))
        game = res.group(3)
        settings[setting].append(round_lifetime(trace_path))

    for setting, samples in settings.items():
        samples = np.array(samples)
        mean = samples.mean()
        sem = st.sem(samples)
        rv = len(samples) -1
        lower, upper = st.t.interval(.99, rv, mean, sem)
        yerr = (upper - lower) / 2
        results.append(dict(x=setting[0], y=setting[1], value=mean, err=yerr))

    return pd.DataFrame(results).set_index(["x", "y"]).unstack()

def round_metrics(path, setting, game):
    trace = pd.read_csv(path,
        comment="#",
        sep=" ",
        usecols=[2, 3, 5] +range(7, 12),
        names=["node", "sent", "timedout", "emit", "reduce", "ram", "normal", "urgent"])

    trace["x"] = setting[0]
    trace["y"] = setting[1]
    trace["game"] = game

    return trace


def metrics(path):
    settings = pd.DataFrame()

    for trace_path in glob.glob(path + "/**/*.csv"):
        res = re.search(r".*/(\d+)-(\d+)/(\d+)n.csv$", trace_path)
        if not res: continue
        
        setting = (int(res.group(1)), int(res.group(2)))
        game = res.group(3)
        settings = settings.append(round_metrics(trace_path, setting, game))

    
    gb = settings.groupby(["x", "y", "node"])
    maximize = ["sent", "timedout", "emit", "reduce"]
    quantilize = ["ram", "normal", "urgent"]
    
    results = pd.DataFrame()
    results[maximize] = gb.max()[maximize]
    results[quantilize] = gb.quantile(.95)[quantilize]
    results["reduced"] = (100 * results.reduce / results.emit)
    results["timedout"] = (100 * results.timedout / results.sent)

    return results
    

def round_delays(path, setting, game):
    trace = pd.read_csv(path,
        comment="#",
        sep=" ",
        usecols=[0, 1, 3, 5],
        names=["time", "node", "urgent", "key"])

    trace["x"] = setting[0]
    trace["y"] = setting[1]
    trace["game"] = game

    return trace


def delay(path):
    settings = pd.DataFrame()
    results = pd.DataFrame()

    for trace_path in glob.glob(path + "/**/*.csv"):
        res = re.search(r".*/(\d+)-(\d+)/(\d+)d.csv$", trace_path)
        if not res: continue
        
        setting = (int(res.group(1)), int(res.group(2)))
        game = res.group(3)
        settings = settings.append(round_delays(trace_path, setting, game))
    
    settings = settings.groupby(["x", "y", "game", "key", "node"]).last()
    # filter urgent pairs
    settings = settings[settings.urgent == 1].unstack()
    
    results["min"] = settings.time.min(axis=1)
    results["max"] = settings.time.max(axis=1)
    results["hops"] = settings.urgent.sum(axis=1)
    results["delay"] = results["max"] - results["min"]
    results["dph"] = results["delay"] / results["hops"]
    results = results.reorder_levels([2, 1, 3, 0]).unstack() 

    return results

life = lifetime(".")
other = metrics(".")
late = delay(".")


ax = life.value.plot(kind="barh", xerr=life.err, ecolor="black")
ax.set_xlabel(u"Tempo de Vida da Rede (dias)")
ax.set_ylabel(u"#Mixes")
ax.get_legend().set_title(u"NCPE")
plt.show()

battery_changes = 365 / life
ax = battery_changes.value.plot(kind="barh", ecolor="black")
ax.set_xlabel(u"Trocas de Bateria por Ano")
ax.set_ylabel(u"#Mixes")
ax.get_legend().set_title(u"NCPE")
plt.show()

life_eficiency = (4. * np.array(life.index) / battery_changes.T - 1).T
ax = life_eficiency.value.plot(kind="barh", ecolor="black")
ax.set_xlabel(u"Ganho Relativo de Tempo de Vida da Rede")
ax.set_ylabel(u"#Mixes")
ax.get_legend().set_title(u"NCPE")
plt.show()

def plot_nodes(metric, xlabel, ylabel, kind="barh"):
    fig, axes = plt.subplots(nrows=1, ncols=3, figsize=(8, 6))
#    fig.subplots_adjust(top=.95, bottom=.075, hspace=.5, right=.72)
    fig.subplots_adjust(wspace=.5, bottom=0.3)
    metric[1].unstack().plot(ax=axes.flat[0], kind=kind, legend=None)

    axes.flat[0].set_title("1")
    axes.flat[0].set_ylabel(xlabel)
    for tick in axes.flat[0].get_xticklabels():
        tick.set_rotation(-45)

    metric[2].unstack().plot(ax=axes.flat[1], kind=kind)
    axes.flat[1].set_ylabel("")
    axes.flat[1].set_title("2")
    axes.flat[1].set_xlabel(ylabel)
    axes.flat[1].legend(loc="lower center", ncol=5, bbox_to_anchor=(.5, -.5))
    axes.flat[1].get_legend().set_title(u"Nós")
    for tick in axes.flat[1].get_xticklabels():
        tick.set_rotation(-45)

    metric[3].unstack().plot(ax=axes.flat[2], kind=kind, legend=None)
    axes.flat[2].set_ylabel("")
    axes.flat[2].set_title("3")
    for tick in axes.flat[2].get_xticklabels():
        tick.set_rotation(-45)

    plt.show()

plot_nodes(other.ram, u"NCPE", u"95-Percentil da Ram Alocada (bytes)")
plot_nodes(100 * other.ram / 10**4, u"NCPE", u"Porcentagem da Ram Total Alocada")
plot_nodes(other.reduced, u"NCPE", u"Percentual de Operações de Fusão")
plot_nodes(other.timedout, u"NCPE", u"Percentual de Mensagens não Confirmadas")

fig, axes = plt.subplots(nrows=2, ncols=2)
late.dph[1].hist(ax=axes.flat[0], bins=35)
axes.flat[0].axvline(x=late.dph[1].quantile(.5), color="g")
axes.flat[0].axvline(x=late.dph[1].quantile(.95), color="g")
axes.flat[0].set_title("1")
axes.flat[0].set_ylabel(u"#Pares")
axes.flat[0].set_xlabel(u"Tempo por Salto (segundos)")

late.dph[2].hist(ax=axes.flat[1], bins=35)
axes.flat[1].axvline(x=late.dph[2].quantile(.5), color="g")
axes.flat[1].axvline(x=late.dph[2].quantile(.95), color="g")
axes.flat[1].set_title("2")
axes.flat[1].set_ylabel(u"#Pares")
axes.flat[1].set_xlabel(u"Tempo por Salto (segundos)")

late.dph[3].hist(ax=axes.flat[2], bins=35)
axes.flat[2].axvline(x=late.dph[3].quantile(.5), color="g")
axes.flat[2].axvline(x=late.dph[3].quantile(.95), color="g")
axes.flat[2].set_title("3")
axes.flat[2].set_ylabel(u"#Pares")
axes.flat[2].set_xlabel(u"Tempo por Salto (segundos)")

late.dph.stack().hist(ax=axes.flat[3], bins=35)
axes.flat[3].axvline(x=late.dph.stack().quantile(.5), color="g")
axes.flat[3].axvline(x=late.dph.stack().quantile(.95), color="g")
axes.flat[3].set_title("Todos")
axes.flat[3].set_ylabel(u"#Pares")
axes.flat[3].set_xlabel(u"Tempo por Salto (segundos)")

plt.tight_layout()
plt.show()

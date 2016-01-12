import sys
import pandas as pd
import numpy as np
import scipy.stats as st
import matplotlib.pyplot as plt

trace = pd.read_csv(sys.argv[1],
        comment="#",
        sep=" ")
#        usecols=[3, 5, 7, 8, 9, 10],
#        names=["time", "node", "cpu", "lpm", "transmit", "listen"])

trace = pd.read_csv("1e.txt", sep=" ", usecols=[0,4,6,7,8], names=["time", "node", "urgent", "pending", "key"], comment="#")
x = trace.set_index(["key", "node"]).time
x.min(level=0)
(x.max(level=0) - x.min(level=0))/10**6

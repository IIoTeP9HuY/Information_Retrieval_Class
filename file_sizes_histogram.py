#!/usr/bin/env python

import fileinput

sizes = []

import os
rootdir = './site'

for subdir, dirs, files in os.walk(rootdir):
	for file in files:
		if file.endswith('.html'):
			sizes.append(os.path.getsize(subdir + '/' + file))

print("Sizes number: {}".format(len(sizes)))

import matplotlib
matplotlib.use('Agg')

import matplotlib.pyplot as plt
fig = plt.figure(figsize=(12, 8))
ax = fig.add_subplot(1, 1, 1)
# ax.set_xscale('log')
ax.set_yscale('log')
plt.hist(sizes)
plt.savefig('histogram.png')

#!/usr/bin/env python3
"""Merges mapping json files: later files win per atlas, except that an atlas is only
replaced by an entry with at least as many exact frames.
usage: texmap_merge.py <out.json> <in1.json> [<in2.json> ...]"""
import json, sys


def exact(e):
    # v3 entries (checked frame by frame) rank above v2 ones (no per-frame check).
    if e.get('v3'):
        return (1, sum(1 for f in e['frames'] if f is not None and f[5] == 'exact'))
    return (0, sum(1 for f in e['frames'] if f is not None and f[1] >= 0))


out = {}
for p in sys.argv[2:]:
    for name, e in json.load(open(p)).items():
        if name not in out or exact(e) >= exact(out[name]):
            out[name] = e
json.dump(out, open(sys.argv[1], 'w'))
print(len(out), 'atlases')

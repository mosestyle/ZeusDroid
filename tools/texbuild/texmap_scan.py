#!/usr/bin/env python3
"""Third pass of the mapping: for every frame that texmap_fix.py could only map
approximately ("approx"/"near"), search all records of the atlas' main file (then of all
files) whose stored size fits the frame, and take an exact match when one exists.
Needed where the original numbering has gaps that the per-atlas base cannot express.
Updates the json named by TEXMAP_JSON (default build/texbuild/texmap.json)."""
import argparse, glob, json, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import texmap_fix as F
G = F.G


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--pack', required=True)
    ap.add_argument('--data', required=True)
    ap.add_argument('--atlases', default='.*', help='regex')
    ap.add_argument('--all-files', action='store_true', help='also search every other file')
    args = ap.parse_args()
    sds = G.load_sprite_data()
    pages = G.load_pack(args.pack)
    pc = {}
    sgs = {os.path.splitext(os.path.basename(p))[0]: F.Sg(p)
           for p in sorted(glob.glob(os.path.join(args.data, '*.sg3')))}
    sizes = {}

    def sizes_of(sg):
        if sg.name not in sizes:
            sizes[sg.name] = [(i, sg.size(i)) for i in range(1, sg.count + 1)]
        return sizes[sg.name]

    jpath = os.environ.get('TEXMAP_JSON', os.path.join(G.ROOT, 'build/texbuild/texmap.json'))
    m = json.load(open(jpath))
    for name in sorted(m):
        if not re.fullmatch(args.atlases, name):
            continue
        e = m[name]
        if not e.get('v3'):
            continue
        todo = [k for k, f in enumerate(e['frames']) if f is not None and f[5] != 'exact']
        if not todo:
            continue
        fixed = 0
        for k in todo:
            t, x, y, w, h, idx = sds[name][k]
            ref = G.page_image(pages, pc, name, t)[y:y + h, x:x + w]
            cur = e['frames'][k]
            order = [sgs[cur[0]]] if cur[0] in sgs else []
            if args.all_files:
                order += [s for n, s in sgs.items() if n != cur[0]]
            hit = None
            for sg in order:
                # nearest indices first
                cands = [i for i, sz in sizes_of(sg)
                         if sz and w <= sz[0] <= w + 8 and h <= sz[1] <= h + 8]
                cands.sort(key=lambda i: abs(i - cur[1]))
                for i in cands:
                    for sh in ((cur[4] == 1), (cur[4] != 1)):
                        q, off = F.quality(sg, i, ref, sh)
                        if q == 0:
                            hit = [sg.name, i, off[0], off[1], 1 if sh else 0, 'exact']
                            break
                    if hit:
                        break
                if hit:
                    break
            if hit:
                e['frames'][k] = hit
                fixed += 1
        left = sum(1 for f in e['frames'] if f is not None and f[5] != 'exact')
        print(f'{name}: {fixed} of {len(todo)} frames now exact, {left} left', flush=True)
        json.dump(m, open(jpath + '.tmp', 'w'))
        os.replace(jpath + '.tmp', jpath)
    print('done', flush=True)


if __name__ == '__main__':
    main()

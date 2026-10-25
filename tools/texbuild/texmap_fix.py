#!/usr/bin/env python3
"""Second pass of the mapping: re-examines atlases that texmap_gen.py could not map
completely. Tries every file / index base (also bases derived from the first frame's size),
with and without shadow conversion, picks the candidate matching most frames and fills the
remaining frames from other candidates. Frames still without an exact source keep the best
candidate's image ("approx"). Updates build/texbuild/texmap.json (format v3: per frame
[file, index, cropX, cropY, shadows, quality])."""
import argparse, ctypes, glob, json, os, re, sys
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import texmap_gen as G

G.lib.sg3_decode.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int,
                             ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.c_int]


class Sg(G.Sg):
    def decode(self, i, shadows=True):
        key = (i, shadows)
        if key in self.cache:
            return self.cache[key]
        w = ctypes.c_int(); h = ctypes.c_int()
        r = G.lib.sg3_decode(self.h, i, G.BUF, len(G.BUF), ctypes.byref(w), ctypes.byref(h), 1 if shadows else 0)
        img = None
        if r == 1 and w.value > 0 and h.value > 0:
            img = np.frombuffer(G.BUF.raw[:w.value * h.value * 4], np.uint8).reshape(h.value, w.value, 4).copy()
        if len(self.cache) > 4000:
            self.cache.clear()
        self.cache[key] = img
        return img


def quality(sg, i, ref, shadows):
    img = sg.decode(i, shadows)
    c, off = G.frame_of(img, ref.shape)
    if c is None or c.shape != ref.shape:
        return 99.0, off
    return G.diff(c, ref), off


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--pack', required=True)
    ap.add_argument('--data', required=True)
    ap.add_argument('--atlases', required=True, help='regex')
    args = ap.parse_args()
    sds = G.load_sprite_data()
    pages = G.load_pack(args.pack)
    pc = {}
    sgs = [Sg(p) for p in sorted(glob.glob(os.path.join(args.data, '*.sg3')))]
    jpath = os.environ.get('TEXMAP_JSON', os.path.join(G.ROOT, 'build/texbuild/texmap.json'))
    m = json.load(open(jpath))
    names = sorted(n for n in set(n for n, _ in pages) if re.fullmatch(args.atlases, n))
    for name in names:
        frames = sds[name]
        refs = []
        for (t, x, y, w, h, idx) in frames:
            refs.append(None if t < 0 or w <= 0 else G.page_image(pages, pc, name, t)[y:y + h, x:x + w])
        valid = [k for k, r in enumerate(refs) if r is not None]
        if not valid:
            continue
        probe = valid[:3]
        cands = []
        for sg in sgs:
            bases = set([0] + [s - 1 for s in sg.bstarts if s > 0])
            # bases where the first probe's size matches some record
            k0 = probe[0]
            rh, rw = refs[k0].shape[:2]
            for i in range(1, sg.count + 1):
                sz = sg.size(i)
                if sz and sz[0] >= rw and sz[1] >= rh and sz[0] <= rw + 8 and sz[1] <= rh + 8:
                    bases.add(i - frames[k0][5])
            for base in bases:
                for shadows in (True, False):
                    ok = True
                    for k in probe:
                        q, _ = quality(sg, base + frames[k][5], refs[k], shadows)
                        if q >= 6:
                            ok = False
                            break
                    if ok:
                        cands.append((sg, base, shadows))
        if not cands:
            print(f'{name}: still no source', flush=True)
            continue
        scored = []
        for (sg, base, shadows) in cands:
            score = 0
            for k in valid:
                q, _ = quality(sg, base + frames[k][5], refs[k], shadows)
                score += 2 if q == 0 else (1 if q < 6 else 0)
            scored.append((score, sg.name, base, shadows, sg))
        scored.sort(key=lambda s: -s[0])
        best = scored[0]
        out = []
        st = {'exact': 0, 'near': 0, 'approx': 0}
        for k, (t, x, y, w, h, idx) in enumerate(frames):
            if refs[k] is None:
                out.append(None)
                continue
            pick = None
            for (_, fname, base, shadows, sg) in scored:
                q, off = quality(sg, base + idx, refs[k], shadows)
                if q < 6:
                    pick = [fname, base + idx, off[0], off[1], 1 if shadows else 0, 'exact' if q == 0 else 'near']
                    break
            if pick is None:
                _, fname, base, shadows, sg = best
                pick = [fname, base + idx, 0, 0, 1 if shadows else 0, 'approx']
            st[pick[5]] += 1
            out.append(pick)
        m[name] = {'file': best[1], 'base': best[2], 'v3': True, 'frames': out}
        print(f'{name}: best {best[1]} base {best[2]} shadows {best[3]} {st}', flush=True)
        # Save after every atlas (long runs may be interrupted).
        json.dump(m, open(jpath + '.tmp', 'w'))
        os.replace(jpath + '.tmp', jpath)


if __name__ == '__main__':
    main()

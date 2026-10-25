#!/usr/bin/env python3
"""Builds the texture-builder mapping (atlas frame -> original sprite) by comparing the
frames of an existing eZeus texture pack with the sprites decoded from the user's own
original game files. Run locally; reads:
  --pack   <dir with interface.e, i30.e>   (local only, never committed)
  --data   <game>/DATA
Writes only indices/rectangles (no pixels): engine/texbuild/etexmap.inc and a local report
build/texbuild/texmap_report.txt.
"""
import argparse, ctypes, glob, io, os, re, sys, json
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
lib = ctypes.CDLL(os.path.join(ROOT, 'build/texbuild/libsg3.so'))
lib.sg3_open.restype = ctypes.c_void_p
for fn in ('sg3_count', 'sg3_nbmp', 'sg3_bstart', 'sg3_bimages', 'sg3_size', 'sg3_decode'):
    getattr(lib, fn).argtypes = None
lib.sg3_count.argtypes = [ctypes.c_void_p]
lib.sg3_nbmp.argtypes = [ctypes.c_void_p]
lib.sg3_bstart.argtypes = [ctypes.c_void_p, ctypes.c_int]
lib.sg3_bimages.argtypes = [ctypes.c_void_p, ctypes.c_int]
lib.sg3_size.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
lib.sg3_decode.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int,
                           ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
BUF = ctypes.create_string_buffer(64 * 1024 * 1024)


class Sg:
    def __init__(self, path):
        self.name = os.path.splitext(os.path.basename(path))[0]
        self.h = lib.sg3_open(path.encode())
        self.count = lib.sg3_count(self.h)
        self.bstarts = [lib.sg3_bstart(self.h, b) for b in range(lib.sg3_nbmp(self.h))]
        self.cache = {}

    def size(self, i):
        w = ctypes.c_int(); h = ctypes.c_int()
        if not lib.sg3_size(self.h, i, ctypes.byref(w), ctypes.byref(h)):
            return None
        return w.value, h.value

    def decode(self, i):
        if i in self.cache:
            return self.cache[i]
        w = ctypes.c_int(); h = ctypes.c_int()
        r = lib.sg3_decode(self.h, i, BUF, len(BUF), ctypes.byref(w), ctypes.byref(h))
        img = None
        if r == 1 and w.value > 0 and h.value > 0:
            img = np.frombuffer(BUF.raw[:w.value * h.value * 4], np.uint8).reshape(h.value, w.value, 4).copy()
        self.cache[i] = img
        return img


def crop(img):
    a = img[:, :, 3]
    ys, xs = np.nonzero(a)
    if len(xs) == 0:
        return None, (0, 0)
    x0, x1, y0, y1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
    return img[y0:y1, x0:x1], (int(x0), int(y0))


def frame_of(img, shape):
    """eZeus frames are the whole original image; a tight crop is tried only
    when the sizes differ."""
    if img is None:
        return None, (0, 0)
    if img.shape[:2] == shape[:2]:
        return img, (0, 0)
    return crop(img)


def diff(a, b):
    """Mean abs RGBA difference (0 = identical)."""
    return float(np.abs(a.astype(np.int16) - b.astype(np.int16)).mean())


def load_sprite_data():
    out = {}
    for f in glob.glob(os.path.join(ROOT, 'engine/spriteData/*30.h')):
        name = os.path.basename(f)[:-4]
        frames = []
        for m in re.finditer(r'eSpriteData\{(-?\d+), (\d+), (\d+), (\d+), (\d+)\}, // (\d+)', open(f).read()):
            t, x, y, w, h, idx = map(int, m.groups())
            frames.append((t, x, y, w, h, idx))
        out[name] = frames
    return out


def load_pack(packdir):
    src = open(os.path.join(ROOT, 'engine/esplitbinary.h')).read()
    ents = re.findall(r'\{"30/([A-Za-z0-9_]+)_(\d+)\.png", eBinaryData\{eFileId::(\w+), (\d+), (\d+)\}\}', src)
    files = {'i': 'interface.e', 'i30': 'i30.e'}
    pages = {}
    handles = {}
    for name, page, fid, pos, size in ents:
        fn = os.path.join(packdir, files[fid])
        if fn not in handles:
            handles[fn] = open(fn, 'rb')
        fh = handles[fn]
        fh.seek(int(pos))
        pages[(name, int(page))] = (fh, int(pos), int(size))
    return pages


def page_image(pages, cache, name, page):
    key = (name, page)
    if key not in cache:
        fh, pos, size = pages[key]
        fh.seek(pos)
        cache[key] = np.array(Image.open(io.BytesIO(fh.read(size))).convert('RGBA'))
    return cache[key]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--pack', required=True)
    ap.add_argument('--data', required=True)
    ap.add_argument('--only', default=None)
    args = ap.parse_args()
    sds = load_sprite_data()
    pages = load_pack(args.pack)
    pcache = {}
    sgs = [Sg(p) for p in sorted(glob.glob(os.path.join(args.data, '*.sg3')))]
    names = sorted(set(n for n, _ in pages))
    if args.only:
        names = [n for n in names if re.fullmatch(args.only, n)]
    jpath = os.environ.get('TEXMAP_JSON', os.path.join(ROOT, 'build/texbuild/texmap.json'))
    result = json.load(open(jpath)) if (args.only and os.path.exists(jpath)) else {}
    report = []
    for name in names:
        frames = sds.get(name)
        if not frames:
            report.append(f'{name}: no sprite data')
            continue
        refs = []
        for k, (t, x, y, w, h, idx) in enumerate(frames):
            if t < 0 or w <= 0 or h <= 0:
                refs.append(None)
                continue
            img = page_image(pages, pcache, name, t)
            refs.append(img[y:y + h, x:x + w])
        probe = [k for k, r in enumerate(refs) if r is not None][:4]
        best = None
        for sg in sgs:
            bases = sorted(set([0] + [s - 1 for s in sg.bstarts if s > 0]))
            for base in bases:
                score = 0
                for k in probe:
                    i = base + frames[k][5]
                    sz = sg.size(i)
                    if not sz:
                        break
                    img = sg.decode(i)
                    if img is None:
                        break
                    c, _ = frame_of(img, refs[k].shape)
                    if c is None or c.shape != refs[k].shape:
                        break
                    if diff(c, refs[k]) > 6:
                        break
                    score += 1
                if score == len(probe) and probe:
                    best = (sg, base)
                    break
            if best:
                break
        if not best:
            report.append(f'{name}: NO SOURCE FOUND ({len(probe)} probes)')
            continue
        sg, base = best
        entries = []
        stats = {'exact': 0, 'near': 0, 'bad': 0, 'empty': 0}
        worst = 0
        for k, (t, x, y, w, h, idx) in enumerate(frames):
            if refs[k] is None:
                entries.append(None)
                stats['empty'] += 1
                continue
            i = base + idx
            img = sg.decode(i)
            c, off = frame_of(img, refs[k].shape)
            if c is None or c.shape != refs[k].shape:
                entries.append((i, -1, -1))
                stats['bad'] += 1
                continue
            d = diff(c, refs[k])
            worst = max(worst, d)
            stats['exact' if d == 0 else ('near' if d < 6 else 'bad')] += 1
            entries.append((i, off[0], off[1]))
        result[name] = {'file': sg.name, 'base': base, 'frames': entries}
        report.append(f'{name}: {sg.name} base {base} {stats} worst {worst:.2f}')
        print(report[-1], flush=True)
    os.makedirs(os.path.join(ROOT, 'build/texbuild'), exist_ok=True)
    rp = 'texmap_report_only.txt' if args.only else 'texmap_report.txt'
    with open(os.path.join(ROOT, 'build/texbuild', rp), 'w') as f:
        f.write('\n'.join(report) + '\n')
    with open(jpath, 'w') as f:
        json.dump(result, f)


if __name__ == '__main__':
    main()

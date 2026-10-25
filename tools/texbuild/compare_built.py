#!/usr/bin/env python3
"""Compares packs made by the texture builder with an existing eZeus pack, frame by
frame (local check only). Usage: compare_built.py <built dir> <eZeus pack dir>"""
import ctypes, glob, io, os, re, sys
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
lib = ctypes.CDLL(os.path.join(ROOT, 'build/texbuild/libsg3.so'))
lib.qoi_decode.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int,
                           ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
BUF = ctypes.create_string_buffer(256 * 1024 * 1024)


def qoi(data):
    w = ctypes.c_int(); h = ctypes.c_int()
    if lib.qoi_decode(data, len(data), BUF, len(BUF), ctypes.byref(w), ctypes.byref(h)) != 1:
        return None
    return np.frombuffer(BUF.raw[:w.value * h.value * 4], np.uint8).reshape(h.value, w.value, 4).copy()


def sprite_data(name, size):
    p = os.path.join(ROOT, 'engine/spriteData', f'{name}{size}.h')
    return [tuple(map(int, m.groups())) for m in
            re.finditer(r'eSpriteData\{(-?\d+), (\d+), (\d+), (\d+), (\d+)\}', open(p).read())]


def main():
    built, pack = sys.argv[1], sys.argv[2]
    only = sys.argv[3] if len(sys.argv) > 3 else None  # optional atlas regex
    src = open(os.path.join(ROOT, 'engine/esplitbinary.h')).read()
    ref = {n: (f, int(p), int(s)) for n, f, p, s in
           re.findall(r'\{"([^"]+)", eBinaryData\{eFileId::(\w+), (\d+), (\d+)\}\}', src)}
    files = {'i': 'interface.e', 'i15': 'i15.e', 'i30': 'i30.e', 'i45': 'i45.e', 'i60': 'i60.e'}
    per_size = {}
    worst = []
    for idx in glob.glob(os.path.join(built, '*.idx')):
        pk = idx[:-4]
        with open(pk, 'rb') as fh:
            for line in open(idx):
                name, pos, size = line.split()
                if name not in ref or not name.endswith('.png'):
                    continue
                if only and not re.fullmatch(only, name.split('/')[1].rsplit('_', 1)[0]):
                    continue
                fh.seek(int(pos)); img = qoi(fh.read(int(size)))
                f, rp, rs = ref[name]
                rpath = os.path.join(pack, files[f])
                if not os.path.exists(rpath):
                    continue
                with open(rpath, 'rb') as rf:
                    rf.seek(rp); rimg = np.array(Image.open(io.BytesIO(rf.read(rs))).convert('RGBA'))
                sz, rest = name.split('/')
                atlas, page = rest[:-4].rsplit('_', 1)
                tot = 0; n = 0; exact = 0
                for (t, x, y, w, h) in sprite_data(atlas, sz):
                    if t != int(page) or w <= 0:
                        continue
                    a = img[y:y + h, x:x + w].astype(np.int16)
                    b = rimg[y:y + h, x:x + w].astype(np.int16)
                    if a.shape != b.shape:
                        continue
                    d = float(np.abs(a - b).mean())
                    tot += d; n += 1; exact += d == 0
                if n:
                    per_size.setdefault(sz, []).append(tot / n)
                    worst.append((tot / n, name, exact, n))
    for sz in sorted(per_size, key=int):
        v = per_size[sz]
        print(f'size {sz}: pages {len(v)} mean diff {np.mean(v):.3f} max {max(v):.3f}')
    worst.sort(reverse=True)
    for w in worst[:12]:
        print('  %.3f %s exact %d/%d' % w)


if __name__ == '__main__':
    main()

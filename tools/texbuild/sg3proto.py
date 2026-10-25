#!/usr/bin/env python3
"""Prototype reader for the original game's SG3/555 sprite files (format notes in
docs/TEXTURE_BUILDER.md). Used only to develop and verify the C++ texture builder;
it prints/returns pixels in memory and never writes game art into the repository."""
import struct, sys, os
import numpy as np

HEADER = 680
BITMAP_REC = 200
MAX_BITMAPS = 200
TRANSPARENT = 0xF81F


class Sg3:
    def __init__(self, path):
        self.path = path
        self.d = open(path, 'rb').read()
        self.version = struct.unpack_from('<I', self.d, 4)[0]
        self.rs = 72 if self.version >= 0xd6 else 64
        self.n = struct.unpack_from('<I', self.d, 16)[0]
        self.nbmp = struct.unpack_from('<I', self.d, 20)[0]
        base = os.path.splitext(path)[0]
        self.data555 = open(base + '.555', 'rb').read()
        self.bitmaps = []
        for b in range(self.nbmp):
            o = HEADER + b * BITMAP_REC
            self.bitmaps.append(self.d[o:o + 65].split(b'\0')[0].decode('latin1'))
        self.dir = os.path.dirname(path)

    def rec(self, i):
        o = HEADER + MAX_BITMAPS * BITMAP_REC + i * self.rs
        d = self.d
        r = dict(offset=struct.unpack_from('<I', d, o)[0],
                 length=struct.unpack_from('<I', d, o + 4)[0],
                 ulength=struct.unpack_from('<I', d, o + 8)[0],
                 mirror=struct.unpack_from('<i', d, o + 16)[0],
                 w=struct.unpack_from('<h', d, o + 20)[0],
                 h=struct.unpack_from('<h', d, o + 22)[0],
                 type=struct.unpack_from('<H', d, o + 50)[0],
                 b52=d[o + 52], b53=d[o + 53], b54=d[o + 54],
                 bmp=d[o + 56], raw=d[o:o + self.rs])
        if self.rs == 72:
            r['aoff'], r['alen'] = struct.unpack_from('<2I', d, o + 64)
        else:
            r['aoff'], r['alen'] = 0, 0
        return r


def c555(p):
    r = (p >> 10) & 0x1f
    g = (p >> 5) & 0x1f
    b = p & 0x1f
    return (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2)


def rle(data, img, w, h, x0=0, y0=0, W=None):
    """RLE sprite: 255 n = skip n transparent pixels, c (<255) = c literal 555 pixels."""
    i = 0
    x = 0
    y = 0
    n = len(data)
    while i < n and y < h:
        c = data[i]
        if c == 255:
            skip = data[i + 1]
            i += 2
            x += skip
        else:
            i += 1
            for _ in range(c):
                p = data[i] | (data[i + 1] << 8)
                i += 2
                if 0 <= y < h:
                    img[y0 + y, x0 + x, :3] = c555(p)
                    img[y0 + y, x0 + x, 3] = 255
                x += 1
                if x >= w:
                    x = 0
                    y += 1
        while x >= w:
            x -= w
            y += 1


def plain(data, w, h):
    a = np.frombuffer(data[:w * h * 2], dtype='<u2').reshape(h, w).astype(np.uint32)
    img = np.zeros((h, w, 4), np.uint8)
    r = (a >> 10) & 0x1f
    g = (a >> 5) & 0x1f
    b = a & 0x1f
    img[:, :, 0] = (r << 3) | (r >> 2)
    img[:, :, 1] = (g << 3) | (g >> 2)
    img[:, :, 2] = (b << 3) | (b >> 2)
    img[:, :, 3] = np.where(a == TRANSPARENT, 0, 255)
    return img


def decode(sg, i):
    r = sg.rec(i)
    if r['offset'] == 0 and r['length'] == 0 and r['mirror']:
        # Mirror entry: the image of record i + mirror, flipped horizontally.
        img = decode(sg, i + r['mirror'])
        return img[:, ::-1].copy() if img is not None else None
    w, h = r['w'], r['h']
    if w <= 0 or h <= 0 or r['length'] == 0:
        return None
    src = sg.data555
    data = src[r['offset']:r['offset'] + r['length']]
    if r['type'] == 30:
        return None  # isometric: see decode_iso (todo)
    if r['length'] == w * h * 2 and r['type'] not in (256, 257, 276):
        img = plain(data, w, h)
    else:
        img = np.zeros((h, w, 4), np.uint8)
        rle(data, img, w, h)
    if r['alen']:
        alpha = src[r['aoff']:r['aoff'] + r['alen']]
        apply_alpha(alpha, img, w, h)
    return img


def apply_alpha(data, img, w, h):
    """Alpha mask, RLE like the sprite data but with 1 byte per value (0..31?)."""
    i = 0
    x = y = 0
    n = len(data)
    while i < n and y < h:
        c = data[i]
        if c == 255:
            skip = data[i + 1]
            i += 2
            x += skip
        else:
            i += 1
            for _ in range(c):
                v = data[i]
                i += 1
                if y < h:
                    img[y, x, 3] = min(255, v * 255 // 31) if img[y, x, 3] == 0 else img[y, x, 3]
                    if img[y, x, 3] and img[y, x, 3] != 255:
                        pass
                x += 1
                if x >= w:
                    x = 0
                    y += 1
        while x >= w:
            x -= w
            y += 1


def crop(img):
    a = img[:, :, 3]
    ys, xs = np.nonzero(a)
    if len(xs) == 0:
        return img[:0, :0], (0, 0)
    x0, x1, y0, y1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
    return img[y0:y1, x0:x1], (x0, y0)


if __name__ == '__main__':
    sg = Sg3(sys.argv[1])
    for i in map(int, sys.argv[2:]):
        r = sg.rec(i)
        print(i, {k: v for k, v in r.items() if k != 'raw'})

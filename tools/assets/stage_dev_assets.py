#!/usr/bin/env python3
"""Stage user-owned Zeus + Poseidon data and the eZeus data pack into the
layout the engine expects, for desktop development/testing or for copying to
an Android device.

Layout produced (same as the Android importer, see ImportRules.java):

    OUT/ZeusPoseidon/                  game root (DATA, Audio, Model, Adventures, *.eng, zeus.ico)
    OUT/ZeusPoseidon/eZeus/            eZeus pack (interface.e, i15.e, i30.e, Fonts, Text, Adventures)
    OUT/ZeusPoseidon/eZeus/Bin/        "binary dir" (EZEUS_BIN_DIR) - empty on Android
    OUT/ZeusPoseidon/eZeus/Zeus_Text.xml, Zeus_MM.xml (converted with eng_to_xml.py)

Original files are never modified. By default files are symlinked (fast, no
extra disk); use --copy for a self-contained folder (e.g. to put on a phone).

Usage:
    stage_dev_assets.py --game "/path/Zeus and Poseidon" --ezeus-zip eZeus-0.8.2-beta.zip --out local-assets
"""
from __future__ import annotations

import argparse
import os
import shutil
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import eng_to_xml  # noqa: E402

GAME_ITEMS = ['DATA', 'Audio', 'Model', 'Adventures', 'Zeus_Text.eng', 'Zeus_MM.eng', 'zeus.ico']
SKIP_IN_PACK = ('bin/', 'vc_redist', 'engconverter', 'save/', 'settings.txt')


def safe_member(name: str):
    n = name.replace('\\', '/')
    if n.startswith('/') or ':' in n.split('/')[0]:
        return None
    parts = [p for p in n.split('/') if p not in ('', '.')]
    if any(p == '..' for p in parts) or len(parts) < 1:
        return None
    return parts


def link_or_copy(src: str, dst: str, copy: bool) -> None:
    if os.path.lexists(dst):
        return
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if copy:
        if os.path.isdir(src):
            shutil.copytree(src, dst)
        else:
            shutil.copy2(src, dst)
    else:
        os.symlink(os.path.abspath(src), dst)


def extract_pack(zpath: str, ezeus_dir: str) -> int:
    n = 0
    with zipfile.ZipFile(zpath) as z:
        total = 0
        for info in z.infolist():
            if info.is_dir():
                continue
            parts = safe_member(info.filename)
            if not parts:
                continue
            if len(parts) == 1:
                if not parts[0].lower().endswith('.e'):
                    continue
                inner = parts[0]
            else:
                if not parts[0].lower().startswith('ezeus'):
                    continue
                inner = '/'.join(parts[1:])
            if inner.lower().startswith(SKIP_IN_PACK):
                continue
            total += info.file_size
            if total > 4 * 1024**3:
                raise RuntimeError('archive expands too much')
            dst = os.path.join(ezeus_dir, *inner.split('/'))
            if os.path.exists(dst) and os.path.getsize(dst) == info.file_size:
                continue
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            tmp = dst + '.part'
            with z.open(info) as src, open(tmp, 'wb') as out:
                shutil.copyfileobj(src, out, 1 << 20)
            os.replace(tmp, dst)
            n += 1
    return n


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--game', required=True, help='Zeus and Poseidon install folder (read-only)')
    ap.add_argument('--ezeus-zip', action='append', default=[], help='eZeus release zip (repeatable: i45.zip, i60.zip)')
    ap.add_argument('--out', required=True)
    ap.add_argument('--copy', action='store_true', help='copy instead of symlink')
    a = ap.parse_args(argv)

    root = os.path.join(a.out, 'ZeusPoseidon')
    ez = os.path.join(root, 'eZeus')
    os.makedirs(os.path.join(ez, 'Bin'), exist_ok=True)
    for item in GAME_ITEMS:
        src = os.path.join(a.game, item)
        if os.path.exists(src):
            link_or_copy(src, os.path.join(root, item), a.copy)
        else:
            print(f'warning: missing {item} in game folder')
    for z in a.ezeus_zip:
        print(f'{z}: extracted {extract_pack(z, ez)} files')
    for name in ('Zeus_Text', 'Zeus_MM'):
        src = os.path.join(a.game, name + '.eng')
        dst = os.path.join(ez, name + '.xml')
        if os.path.exists(src) and not os.path.exists(dst):
            with open(src, 'rb') as f:
                eng_to_xml.atomic_write(dst, eng_to_xml.convert_bytes(f.read()))
            print(f'converted {name}.eng')
    print(f'staged into {root}; run the desktop build with EZEUS_BIN_DIR={os.path.join(ez, "Bin")}')
    return 0


if __name__ == '__main__':
    sys.exit(main())

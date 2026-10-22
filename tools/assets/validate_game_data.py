#!/usr/bin/env python3
"""Validates a user-owned Zeus + Poseidon installation (and optionally a staged
eZeus layout) against tools/assets/asset_manifest.json, or (re)generates that
manifest from a known-good installation.

The manifest contains only relative paths, sizes, required flags, SHA-256
digests of a few small identifying files, and version metadata. It never
contains game content, so it is safe to commit.

Usage:
    validate_game_data.py check   GAME_DIR [--ezeus EZEUS_DIR] [--manifest M] [--json]
    validate_game_data.py generate GAME_DIR [--listing device_listing.json] [--manifest M]

Exit codes: 0 = all required files present (and sizes match),
            1 = usage / IO error, 2 = required files missing or mismatched.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_MANIFEST = os.path.join(HERE, 'asset_manifest.json')

# Directories / files the engine reads (see docs/GAME_DATA.md).
REQUIRED_PREFIXES = ('DATA/', 'Model/', 'Adventures/')
REQUIRED_FILES = ('Zeus_Text.eng', 'Zeus_MM.eng')
OPTIONAL_PREFIXES = ('Audio/',)
OPTIONAL_FILES = ('zeus.ico',)
# Small files whose digest identifies the edition (safe to publish digests).
HASHED = ('Zeus.exe', 'Zeus_Text.eng', 'Zeus_MM.eng', 'DATA/Zeus_Terrain.sg3', 'DATA/SprMain.sg3')
# Files from the eZeus release pack (eZeus-0.8.2-beta.zip).
EZEUS_REQUIRED = ('interface.e', 'Fonts/Zeus.ttf', 'Text/language.txt')
EZEUS_ONE_OF = (('i15.e', 'i30.e'),)
EZEUS_OPTIONAL = ('i45.e', 'i60.e', 'Zeus_Text.xml', 'Zeus_MM.xml')


def sha256(path: str) -> str:
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def classify(rel: str):
    if rel in REQUIRED_FILES or rel.startswith(REQUIRED_PREFIXES):
        return True
    if rel in OPTIONAL_FILES or rel.startswith(OPTIONAL_PREFIXES):
        return False
    return None  # not used by the engine


def walk(game: str):
    for dp, _dn, fn in os.walk(game, followlinks=True):
        for n in fn:
            full = os.path.join(dp, n)
            yield os.path.relpath(full, game).replace(os.sep, '/'), full


def generate(game: str, listing: str | None, manifest: str) -> int:
    entries = {}
    if listing:
        # Device listing JSON ({"entries":[{"name","type","size"}]}) lets the
        # manifest include files that were not copied locally (e.g. audio).
        with open(listing) as f:
            for e in json.load(f)['entries']:
                if e.get('type') != 'file':
                    continue
                rel = e['name'].replace('\\', '/')
                req = classify(rel)
                if req is None:
                    continue
                entries[rel] = {'size': e['size'], 'required': req}
    for rel, full in walk(game):
        req = classify(rel)
        if req is None and rel not in HASHED:
            continue
        ent = entries.setdefault(rel, {'size': os.path.getsize(full), 'required': bool(req)})
        ent['size'] = os.path.getsize(full)
        if rel in HASHED:
            ent['sha256'] = sha256(full)
    out = {
        'format': 1,
        'description': 'Zeus: Master of Olympus + Poseidon (GOG, English) file manifest. '
                       'Names/sizes/digests only - no game content.',
        'edition': {'store': 'GOG', 'gog_game_id': '1207659039', 'language': 'english',
                    'exe_version_string': 'Version 1.11', 'exe_pe_timestamp': '2002-01-04T11:03:23Z',
                    'exe_machine': 'i386 (PE32)'},
        'ezeus_pack': {'release': '0.8.2-beta', 'required': list(EZEUS_REQUIRED),
                       'one_of': [list(x) for x in EZEUS_ONE_OF], 'optional': list(EZEUS_OPTIONAL)},
        'files': dict(sorted(entries.items())),
    }
    with open(manifest, 'w') as f:
        json.dump(out, f, indent=1, sort_keys=False)
        f.write('\n')
    req = sum(1 for e in entries.values() if e['required'])
    print(f'wrote {manifest}: {len(entries)} files ({req} required)')
    return 0


def check(game: str, ezeus: str | None, manifest: str, as_json: bool) -> int:
    with open(manifest) as f:
        m = json.load(f)
    missing, wrong_size, wrong_hash, optional_missing = [], [], [], []
    # Case-insensitive lookup: Windows installs and Android shared storage are
    # case-insensitive, Linux is not; report both.
    present = {}
    for rel, full in walk(game):
        present.setdefault(rel.lower(), (rel, full))
    case_mismatch = []
    for rel, e in m['files'].items():
        hit = present.get(rel.lower())
        if not hit:
            (missing if e['required'] else optional_missing).append(rel)
            continue
        actual_rel, full = hit
        if actual_rel != rel:
            case_mismatch.append((rel, actual_rel))
        if os.path.getsize(full) != e['size']:
            wrong_size.append(rel)
        elif 'sha256' in e and sha256(full) != e['sha256']:
            wrong_hash.append(rel)
    ez_missing = []
    if ezeus:
        for r in m['ezeus_pack']['required']:
            if not os.path.isfile(os.path.join(ezeus, r)):
                ez_missing.append(r)
        for group in m['ezeus_pack']['one_of']:
            if not any(os.path.isfile(os.path.join(ezeus, r)) for r in group):
                ez_missing.append(' or '.join(group))
    report = {
        'manifest_files': len(m['files']),
        'missing_required': missing,
        'missing_optional': len(optional_missing),
        'size_mismatch': wrong_size,
        'digest_mismatch': wrong_hash,
        'case_mismatch': case_mismatch,
        'ezeus_missing': ez_missing,
    }
    ok = not (missing or wrong_size or ez_missing)
    report['ok'] = ok
    if as_json:
        print(json.dumps(report, indent=1))
    else:
        print(f"files in manifest: {report['manifest_files']}")
        print(f"missing required: {len(missing)}  optional missing: {len(optional_missing)}")
        for r in missing[:20]:
            print('  MISSING', r)
        for r in wrong_size[:20]:
            print('  SIZE MISMATCH', r)
        for r in wrong_hash[:20]:
            print('  DIGEST DIFFERS (different edition/patch?)', r)
        for a, b in case_mismatch[:20]:
            print(f'  CASE {a} -> {b}')
        for r in ez_missing:
            print('  EZEUS MISSING', r)
        print('RESULT:', 'OK' if ok else 'INCOMPLETE')
    return 0 if ok else 2


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    sub = ap.add_subparsers(dest='cmd', required=True)
    c = sub.add_parser('check')
    c.add_argument('game')
    c.add_argument('--ezeus')
    c.add_argument('--manifest', default=DEFAULT_MANIFEST)
    c.add_argument('--json', action='store_true')
    g = sub.add_parser('generate')
    g.add_argument('game')
    g.add_argument('--listing')
    g.add_argument('--manifest', default=DEFAULT_MANIFEST)
    a = ap.parse_args(argv)
    if not os.path.isdir(a.game):
        print(f'error: not a directory: {a.game}', file=sys.stderr)
        return 1
    if a.cmd == 'generate':
        return generate(a.game, a.listing, a.manifest)
    return check(a.game, a.ezeus, a.manifest, a.json)


if __name__ == '__main__':
    sys.exit(main())

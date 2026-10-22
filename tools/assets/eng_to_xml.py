#!/usr/bin/env python3
"""Portable converter for Impressions Games .eng language files (Zeus/Poseidon).

Produces the same XML that eZeus expects (``Zeus_Text.xml`` / ``Zeus_MM.xml``),
replacing the Windows-only ``engconverter-0.5.exe`` shipped with eZeus releases.

The binary layout and XML shape are reimplemented from the MIT-licensed
citybuilding-tools engconverter by Bianca van Schaik
(https://github.com/bvschaik/citybuilding-tools, engconverter/src), see
LICENSES/citybuilding-tools-MIT.txt.  Output formatting mimics Qt's
QXmlStreamWriter with auto-formatting (4-space indent, UTF-8, escaping of
< > & and ") because eZeus' line-based parser (engine/exmlparser.cpp)
depends on one element per line.

Usage:
    eng_to_xml.py INPUT.eng OUTPUT.xml [--encoding cp1252]

Exit status: 0 ok, 1 usage/IO error, 2 unrecognised or corrupt file.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys
import tempfile
from dataclasses import dataclass, field
from typing import List, Optional

TEXT_MAX_INDEX_ENTRIES = 1000
MAX_DATA_SIZE = 1_000_000

TYPE_UNKNOWN, TYPE_TEXT, TYPE_MESSAGE = 0, 1, 2


class EngError(Exception):
    pass


def _escape(text: str) -> str:
    out = []
    for c in text:
        if c == '<':
            out.append('&lt;')
        elif c == '>':
            out.append('&gt;')
        elif c == '&':
            out.append('&amp;')
        elif c == '"':
            out.append('&quot;')
        else:
            out.append(c)
    return ''.join(out)


def _cstr(raw: bytes, start: int, encoding: str) -> str:
    end = raw.find(b'\0', start)
    if end < 0:
        end = len(raw)
    return raw[start:end].decode(encoding, errors='replace')


def detect_type(data: bytes) -> int:
    if len(data) < 24:
        return TYPE_UNKNOWN
    first, second = struct.unpack_from('<ii', data, 16)
    if first in (400, 1000) and first >= second:
        return TYPE_MESSAGE
    if first <= 400 and first <= second:
        return TYPE_TEXT
    off = 16 + 8 + 8000
    if len(data) >= off + 8:
        zero, nonzero = struct.unpack_from('<II', data, off)
        if zero == 0 and nonzero != 0:
            return TYPE_TEXT
    return TYPE_UNKNOWN


def _trim_data(raw: bytes) -> int:
    size = len(raw)
    while size > 1 and raw[size - 1] == 0 and raw[size - 2] == 0:
        size -= 1
    return size


# ---------------------------------------------------------------- text files

@dataclass
class TextGroup:
    id: int
    offset: int
    strings: List[str] = field(default_factory=list)


def read_text(data: bytes, encoding: str):
    name = ''.join(chr(b) if b < 0x80 else '?' for b in data[:16].split(b'\0', 1)[0])
    pos = 16 + 12
    groups: List[TextGroup] = []
    index_with_counts = False
    need = pos + TEXT_MAX_INDEX_ENTRIES * 8
    if len(data) < need:
        raise EngError('text file truncated in index table')
    for gid in range(TEXT_MAX_INDEX_ENTRIES):
        offset, used = struct.unpack_from('<ii', data, pos)
        pos += 8
        if used:
            groups.append(TextGroup(gid, offset))
            if used > 1:
                index_with_counts = True
    raw = data[pos:pos + MAX_DATA_SIZE]
    size = _trim_data(raw)
    for i, g in enumerate(groups):
        start = g.offset
        end = size if i + 1 == len(groups) else groups[i + 1].offset
        if start > size or end > size or start > end or start < 0:
            raise EngError(f'invalid data offset for group {g.id}: {start}-{end}')
        while start < end:
            while start < end and raw[start] == 0:
                start += 1
            g.strings.append(_cstr(raw, start, encoding))
            while start < len(raw) and raw[start] != 0:
                start += 1
            start += 1
    return name, index_with_counts, groups


def write_text_xml(name, index_with_counts, groups) -> str:
    lines = ['<?xml version="1.0" encoding="UTF-8"?>',
             f'<strings name="{_escape(name)}" indexWithCounts="{"true" if index_with_counts else "false"}">']
    for g in groups:
        lines.append(f'    <group id="{g.id}">')
        for i, s in enumerate(g.strings):
            lines.append(f'        <string id="{i}">{_escape(s)}</string>')
        lines.append('    </group>')
    lines.append('</strings>')
    return '\n'.join(lines) + '\n'


# ------------------------------------------------------------- message files

@dataclass
class Msg:
    id: int
    type: int = 0
    subtype: int = 0
    dialog: tuple = (0, 0, 0, 0)
    image: tuple = (0, 0, 0)
    image2: tuple = (0, 0, 0)
    title_xy: tuple = (0, 0)
    subtitle_xy: tuple = (0, 0)
    video_xy: tuple = (0, 0)
    urgent: int = 0
    offsets: dict = field(default_factory=dict)
    texts: dict = field(default_factory=dict)

    def is_empty(self) -> bool:
        if self.image[0] or self.image2[0]:
            return False
        if any(self.dialog):
            return False
        return all(self.offsets[k] == 0 for k in ('video', 'title', 'subtitle', 'content'))


MSG_ENTRY = struct.Struct('<hhh hhhh hhh hhh hh hh 4x hh 14x i i 4x i i i')
assert MSG_ENTRY.size == 80


def read_messages(data: bytes, encoding: str):
    name = ''.join(chr(b) if b < 0x80 else '?' for b in data[:16].split(b'\0', 1)[0])
    total, _used = struct.unpack_from('<ii', data, 16)
    if total < 0 or total > 100000:
        raise EngError(f'implausible message count {total}')
    pos = 24
    if len(data) < pos + total * MSG_ENTRY.size:
        raise EngError('message file truncated in entry table')
    entries: List[Msg] = []
    for mid in range(total):
        f = MSG_ENTRY.unpack_from(data, pos)
        pos += MSG_ENTRY.size
        m = Msg(mid, f[0], f[1], (f[3], f[4], f[5], f[6]), (f[7], f[8], f[9]),
                (f[10], f[11], f[12]), (f[13], f[14]), (f[15], f[16]), (f[17], f[18]),
                f[19], {'video': f[20], 'title': f[21], 'subtitle': f[22], 'content': f[23]})
        if not m.is_empty():
            entries.append(m)
    raw = data[pos:pos + MAX_DATA_SIZE]
    size = _trim_data(raw)
    for m in entries:
        for k, off in m.offsets.items():
            if off:
                if off < 0 or off > size:
                    raise EngError(f'invalid data offset {off} for {k} text in entry {m.id}')
                m.texts[k] = _cstr(raw, off, encoding)
            else:
                m.texts[k] = ''
    return name, total, entries


def write_messages_xml(name, total, entries) -> str:
    L = ['<?xml version="1.0" encoding="UTF-8"?>',
         f'<messages name="{_escape(name)}" entries="{total}">']
    for m in entries:
        urgent = ' urgent="true"' if m.urgent else ''
        L.append(f'    <message id="{m.id}" type="{m.type}" subtype="{m.subtype}"{urgent}>')
        d = m.dialog
        L.append(f'        <dialog x="{d[0]}" y="{d[1]}" width="{d[2]}" height="{d[3]}"/>')
        if m.image[0]:
            L.append(f'        <image graphic="{m.image[0]}" x="{m.image[1]}" y="{m.image[2]}"/>')
        if m.image2[0]:
            # Mirrors upstream engconverter, which writes image2 tag with image() values.
            L.append(f'        <image2 graphic="{m.image[0]}" x="{m.image[1]}" y="{m.image[2]}"/>')
        for tag, xy in (('title', m.title_xy), ('subtitle', m.subtitle_xy), ('video', m.video_xy)):
            if m.offsets[tag] == 0 and not m.texts[tag]:
                continue
            L.append(f'        <{tag} x="{xy[0]}" y="{xy[1]}">{_escape(m.texts[tag])}</{tag}>')
        if m.texts['content']:
            L.append(f'        <content>{_escape(m.texts["content"].replace(chr(0xe), "~"))}</content>')
        L.append('    </message>')
    L.append('</messages>')
    return '\n'.join(L) + '\n'


def convert_bytes(data: bytes, encoding: str = 'cp1252') -> str:
    t = detect_type(data)
    if t == TYPE_TEXT:
        return write_text_xml(*read_text(data, encoding))
    if t == TYPE_MESSAGE:
        return write_messages_xml(*read_messages(data, encoding))
    raise EngError('unknown .eng file type')


def atomic_write(path: str, text: str) -> None:
    d = os.path.dirname(os.path.abspath(path))
    os.makedirs(d, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=d, prefix='.tmp-', suffix='.xml')
    try:
        with os.fdopen(fd, 'w', encoding='utf-8', newline='\n') as f:
            f.write(text)
        os.replace(tmp, path)
    except BaseException:
        if os.path.exists(tmp):
            os.unlink(tmp)
        raise


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('input')
    ap.add_argument('output')
    ap.add_argument('--encoding', default='cp1252',
                    help='source text encoding (default cp1252 = engconverter "Windows-1252 - Default")')
    a = ap.parse_args(argv)
    try:
        with open(a.input, 'rb') as f:
            data = f.read()
    except OSError as e:
        print(f'error: cannot read {a.input}: {e}', file=sys.stderr)
        return 1
    try:
        xml = convert_bytes(data, a.encoding)
    except EngError as e:
        print(f'error: {a.input}: {e}', file=sys.stderr)
        return 2
    atomic_write(a.output, xml)
    print(f'wrote {a.output} ({len(xml)} chars)')
    return 0


if __name__ == '__main__':
    sys.exit(main())

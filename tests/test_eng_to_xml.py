#!/usr/bin/env python3
"""Tests for tools/assets/eng_to_xml.py and the native converter using
synthetic .eng files (no copyrighted data needed). Optionally compares both
converters on real files if ZEUS_GAME_DIR is set."""
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'assets'))
import eng_to_xml  # noqa: E402

fails = 0


def check(cond, what):
    global fails
    if not cond:
        fails += 1
        print('  FAIL', what)


def make_text(groups):
    """groups: {id: [str,...]} -> .eng bytes (text file layout)."""
    data = b''
    index = [(0, 0)] * 1000
    for gid in sorted(groups):
        off = len(data)
        for s in groups[gid]:
            data += s.encode('cp1252') + b'\0'
        index[gid] = (off, len(groups[gid]))
    hdr = b'Test textfile.\0\0' + struct.pack('<iii', max(groups) + 1, 0, 0)
    return hdr + b''.join(struct.pack('<ii', o, u) for o, u in index) + data + b'\0'


def make_messages(msgs, total=400):
    text = b'\0' * 16
    entries = []
    for mid in range(total):
        if mid in msgs:
            title, content = msgs[mid]
            toff = len(text); text += title.encode('cp1252') + b'\0'
            coff = len(text); text += content.encode('cp1252') + b'\0'
            e = struct.pack('<hhh hhhh hhh hhh hh hh 4x hh 14x i i 4x i i i',
                            1, 0, 0, 0, 24, 28, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, toff, 0, coff)
        else:
            e = b'\0' * 80
        entries.append(e)
    return b'Test messages.\0\0' + struct.pack('<ii', total, len(msgs)) + b''.join(entries) + text


def main():
    native = sys.argv[1] if len(sys.argv) > 1 else None
    t = make_text({1: ['File', 'New <adventure> & "more"', 'Caf\xe9'], 3: ['x']})
    xml = eng_to_xml.convert_bytes(t)
    check('<group id="1">' in xml and '<group id="3">' in xml, 'groups present')
    check('New &lt;adventure&gt; &amp; &quot;more&quot;' in xml, 'escaping')
    check('Caf\xe9' in xml, 'cp1252 decoded')
    check('indexWithCounts="true"' in xml, 'counts flag')
    m = make_messages({0: ('About', 'Hello @Lworld'), 5: ('T', 'C~')})
    mx = eng_to_xml.convert_bytes(m)
    check('<message id="0" type="1"' in mx and '<message id="5"' in mx, 'messages present')
    check('<title x="0" y="0">About</title>' in mx, 'title')
    check('<dialog x="0" y="24" width="28" height="17"/>' in mx, 'dialog')
    check(mx.count('<message ') == 2, 'empty entries skipped')
    try:
        eng_to_xml.convert_bytes(b'garbage')
        check(False, 'garbage rejected')
    except eng_to_xml.EngError:
        pass
    if native:
        with tempfile.TemporaryDirectory() as d:
            for name, blob in (('t', t), ('m', m)):
                src = os.path.join(d, name + '.eng')
                out = os.path.join(d, name + '.xml')
                open(src, 'wb').write(blob)
                r = subprocess.run([native, src, out])
                check(r.returncode == 0, f'native converter ok ({name})')
                check(open(out, encoding='utf-8').read() == eng_to_xml.convert_bytes(blob), f'native == python ({name})')
            game = os.environ.get('ZEUS_GAME_DIR')
            if game:
                for f in ('Zeus_Text.eng', 'Zeus_MM.eng'):
                    src = os.path.join(game, f)
                    out = os.path.join(d, f + '.xml')
                    subprocess.run([native, src, out], check=True)
                    same = open(out, 'rb').read() == eng_to_xml.convert_bytes(open(src, 'rb').read()).encode('utf-8')
                    check(same, f'real {f}: native == python')
    print(f'eng_to_xml: {"OK" if fails == 0 else str(fails) + " failures"}')
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())

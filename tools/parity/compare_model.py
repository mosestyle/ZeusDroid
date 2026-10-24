#!/usr/bin/env python3
"""Compare eZeus' built-in balancing approximations with the original game's
Model/Zeus_Model_<Difficulty>.txt files (the user's own install).

usage: compare_model.py <game root with Model/> [out.md]

eZeus computes cost / fire risk / damage risk as base(type) * factor(difficulty)
(engine/engine/edifficulty.cpp). The original has one value per building and
difficulty. The report (default build/parity/model_compare.md) contains numbers
from the original files and therefore stays local (build/ is git-ignored).
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ENG = os.path.join(ROOT, 'engine')
DIFFS = ['beginner', 'mortal', 'hero', 'titan', 'olympian']
FILES = ['VeryEasy', 'Easy', 'Normal', 'Hard', 'Impossible']


def src(rel):
    return open(os.path.join(ENG, rel), encoding='utf-8-sig').read()


def parse_switch(text, func):
    """{type: value} from `int func(const eBuildingType type) { switch ... }`."""
    m = re.search(r'\n(?:int|double) ' + re.escape(func) + r'\([^)]*\) \{(.*?)\n\}', text, re.S)
    body = m.group(1)
    out, pending = {}, []
    for line in body.splitlines():
        line = line.strip()
        cm = re.match(r'case eBuildingType::(\w+):', line)
        if cm:
            pending.append(cm.group(1)); continue
        rm = re.match(r'return ([\d.]+);', line)
        if rm:
            for t in pending: out[t] = float(rm.group(1))
            pending = []
        if line.startswith('default'):
            pending = ['__default__']
    return out


def parse_mult(text, func):
    m = re.search(r'\ndouble ' + re.escape(func) + r'\([^)]*\) \{(.*?)\n\}', text, re.S)
    out = {}
    for d, v in re.findall(r'case eDifficulty::(\w+):\s*return ([\d.]+);', m.group(1)):
        out[d] = float(v)
    return out


def ezeus_round(x):
    # std::round: half away from zero
    import math
    return int(math.floor(x + 0.5)) if x >= 0 else -int(math.floor(-x + 0.5))


def name_map():
    t = src('engine/emodeldata.cpp')
    block = t.split('MODEL_NAME_MAP_BEGIN')[1].split('MODEL_NAME_MAP_END')[0]
    return re.findall(r'\{"(BUILD_\w+)", eT::(\w+)\}', block)


def parse_model(path):
    data, inb = {}, False
    for line in open(path, encoding='latin-1'):
        if line.startswith('ALL BUILDINGS'): inb = True; continue
        if line.startswith('ALL HOUSES'): break
        if not inb or ',{,' not in line: continue
        head, rest = line.split(',{,', 1)
        name = head.split(',', 1)[1].strip().strip('"')
        name = re.split(r'[ ,(]', name)[0]
        vals = [int(x) for x in rest.split(',}')[0].split(',')]
        data[name] = vals
    return data


def main():
    game = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'build/parity/model_compare.md')
    d = src('engine/edifficulty.cpp')
    cost = parse_switch(d, 'costBase'); cmul = parse_mult(d, 'costMultiplier')
    fire = parse_switch(d, 'fireRiskBase'); fmul = parse_mult(d, 'fireRiskMultiplier')
    dmg = parse_switch(d, 'damageRiskBase'); dmul = parse_mult(d, 'damageRiskMultiplier')
    temples = set(t for _, t in name_map() if t.startswith('temple'))
    models = [parse_model(os.path.join(game, 'Model', 'Zeus_Model_%s.txt' % f)) for f in FILES]
    rows, counts = [], {'cost': [0, 0], 'fire': [0, 0], 'damage': [0, 0]}
    for bname, t in name_map():
        for i, diff in enumerate(DIFFS):
            mv = models[i].get(bname)
            if not mv: continue
            def ez(table, mul):
                base = table.get(t, table.get('__default__', 0))
                if table is cost and t in temples: return int(base)
                return ezeus_round(base * mul[diff])
            for key, idx, table, mul in (('cost', 0, cost, cmul), ('fire', 6, fire, fmul), ('damage', 7, dmg, dmul)):
                e, o = ez(table, mul), mv[idx]
                counts[key][0] += 1
                if e != o:
                    counts[key][1] += 1
                    rows.append((key, t, diff, e, o))
    # desirability (value, range): eZeus table vs original columns b and e
    ht = src('buildings/eheatgetters.cpp')
    body = ht.split('eHeat eZeusAppeal(const eBuildingType type) {')[1].split('\n}\n')[0]
    ez, pend = {}, []
    for line in body.splitlines():
        pend += re.findall(r'case eBuildingType::(\w+):', line)
        rm = re.search(r'return \{(-?\d+), (\d+)\}', line)
        if rm:
            for p_ in pend: ez[p_] = (int(rm.group(1)), int(rm.group(2)))
            pend = []
    counts['appeal'] = [0, 0]
    for bname, t in name_map():
        mv = models[2].get(bname)
        if not mv or t not in ez: continue
        counts['appeal'][0] += 1
        if ez[t] != (mv[1], mv[4]):
            counts['appeal'][1] += 1
            rows.append(('appeal (value, range)', t, 'all', '%d, %d' % ez[t], '%d, %d' % (mv[1], mv[4])))
    # employees: constructor literals "eBuildingType::X, w, h, N" and sanctuaries
    import glob
    emp = {}
    for f in glob.glob(os.path.join(ENG, 'buildings', '**', '*.cpp'), recursive=True):
        s_ = open(f, encoding='utf-8-sig').read()
        for mm in re.finditer(r'eBuildingType::(\w+)\s*,\s*(?:\d+|sw)\s*,\s*(?:\d+|sh)\s*,\s*(\d+)', s_):
            emp.setdefault(mm.group(1), int(mm.group(2)))
    counts['employees'] = [0, 0]
    for bname, t in name_map():
        mv = models[2].get(bname)
        if not mv or t not in emp or mv[5] <= 0 or emp[t] <= 0: continue
        counts['employees'][0] += 1
        if emp[t] != mv[5]:
            counts['employees'][1] += 1
            rows.append(('employees', t, 'all', emp[t], mv[5]))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w') as f:
        f.write('# eZeus vs original model data (local report, contains original values)\n\n')
        for k, (n, bad) in counts.items():
            f.write('* %s: %d of %d compared values differ\n' % (k, bad, n))
        f.write('\n| value | building | difficulty | eZeus | original |\n|---|---|---|---|---|\n')
        for r in rows: f.write('| %s | %s | %s | %s | %s |\n' % r)
    for k, (n, bad) in counts.items():
        print('%s: %d of %d pairs differ' % (k, bad, n))
    print('report:', out)


if __name__ == '__main__':
    main()

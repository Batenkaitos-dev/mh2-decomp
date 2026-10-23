#!/usr/bin/env python3
"""Function declarations that disagree with their definitions, in every module.

The PS2 passes floats and integers in separate registers, and MWCC passes a
narrower or wider integer the same way in most calls, so the game's code
matches with declarations that disagree with their definitions. A C-built
port passes arguments by position and by type: each disagreement is a bug
there. This lists them, declaration by declaration (file scope and block
scope, headers included), against the definition the declaration's module
links to (the same file's static function, the module's own, or main's and
sub_main's, which every module sees).

Kinds, most serious first (a declaration gets the first that applies):
  count    another number of parameters (`(void)` against parameters too)
  unspec   declared `()`, defined with parameters (K&R-style calls)
  order    the same number, int and float parameters in another order
  return   void against a value, int against float or pointer
  ptrint   a pointer parameter declared an integer, or the reverse
  width    an integer parameter of another width or sign
  retw     a return type of another integer width or sign, or pointer type
  ptr      a pointer parameter to another type (`void *`, `s32 *` ...)
  other    any other parameter type difference (structs, arrays, ...)
  unit     any of the above, marked as the game's own disagreement by a comment
           `/* unit's own declaration: <evidence> */` on the declaration's line or
           the line before it (the calling code matches only with it); counted
           apart, so the kinds above are the ones still to fix

usage: proto_check.py [REPO] [--list KIND[,KIND]] [--module M] [--file SUBSTR]
                      [--by-module] [--callee NAME]
Without --list it prints one summary line per kind (and per module with
--by-module).
"""
import re, sys, os, glob, collections, argparse

KW = {'if', 'while', 'for', 'switch', 'return', 'sizeof', 'else', 'do', 'case',
      'goto', 'typedef', 'INCLUDE_ASM', 'INCLUDE_RODATA', 'defined'}
INTS = {  # name -> (size, signed)
    's8': (1, 1), 'u8': (1, 0), 's16': (2, 1), 'u16': (2, 0), 's32': (4, 1),
    'u32': (4, 0), 's64': (8, 1), 'u64': (8, 0), 'char': (1, 1),
    'signed char': (1, 1), 'unsigned char': (1, 0), 'short': (2, 1),
    'signed short': (2, 1), 'unsigned short': (2, 0), 'short int': (2, 1),
    'unsigned short int': (2, 0), 'int': (4, 1), 'signed': (4, 1),
    'signed int': (4, 1), 'unsigned': (4, 0), 'unsigned int': (4, 0),
    'long': (8, 1), 'unsigned long': (8, 0), 'long int': (8, 1),   # MWCC for the EE: long is 64-bit
    'long long': (8, 1), 'unsigned long long': (8, 0), 'size_t': (4, 0),
    'BOOL': (4, 1), 'bool': (4, 1), 'u_int': (4, 0), 'u_char': (1, 0),
    'u_short': (2, 0), 'u_long': (8, 0), 'u_long128': (16, 0), 'u128': (16, 0),
}
FLOATS = {'f32': 'f32', 'float': 'f32', 'f64': 'f64', 'double': 'f64'}
FNPTR = {}  # function pointer typedef -> its spelled-out type
CANON = {}  # typedef alias -> canonical spelling (filled from the headers)

def blank(m):
    return re.sub(r'[^\n]', ' ', m.group(0))

# Comments and literals in one left-to-right pass, so that a "/*" inside a string and an
# apostrophe inside a comment are each read as what they are.
TOKEN = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)


def strip_comments(src):
    def one(m):
        s = m.group(0)
        if s[0] == '/':
            return blank(m)
        # a string or char literal: keep its length, drop its content
        return s[0] + ' ' * (len(s) - 2) + s[0]
    return TOKEN.sub(one, src)

def split_top(p):
    out, d, cur = [], 0, ''
    for c in p:
        if c in '([': d += 1
        elif c in ')]': d -= 1
        if c == ',' and d == 0:
            out.append(cur); cur = ''
        else:
            cur += c
    out.append(cur)
    return out

def norm_type(a, drop_name=True):
    """'const Em *em' -> 'Em*'; function pointers -> 'ret(*)(...)'."""
    a = ' '.join(a.split())
    if '(' in a:
        m = re.match(r'^(.*?)\(\s*\*\s*\w*\s*(\[[^\]]*\])?\s*\)\s*\((.*)\)$', a)
        if m:
            ps = norm_params(m.group(3))
            return (norm_type(m.group(1), drop_name=False) + '(*)' +
                    ('()' if ps is None else '(' + ','.join(ps) + ')') + ('*' if m.group(2) else ''))
        a = re.sub(r'\(\s*\*\s*\w*\s*\)', '(*)', a)
        return a.replace(' ', '')
    arr = len(re.findall(r'\[[^\]]*\]', a))
    a = re.sub(r'\[[^\]]*\]', ' ', a)
    toks = a.replace('*', ' * ').split()
    toks = [t for t in toks if t not in ('const', 'volatile', 'register', 'static', 'extern', 'inline', '__inline', 'asm')]
    if drop_name and len(toks) > 1 and re.match(r'^[A-Za-z_]\w*$', toks[-1]):
        base = ' '.join(toks[:-1])
        if not (base in ('struct', 'union', 'enum') or ' '.join(toks) in INTS or toks[-1] in INTS or toks[-1] in FLOATS):
            toks = toks[:-1]
    stars = toks.count('*') + arr
    base = ' '.join(t for t in toks if t != '*')
    if base.startswith(('struct ', 'union ')) and len(base.split()) == 2:
        base = base.split()[1]  # the tag: the project names typedefs after their tags
    base = CANON.get(base, base)
    if base in INTS:
        base = {(1, 1): 's8', (1, 0): 'u8', (2, 1): 's16', (2, 0): 'u16', (4, 1): 's32',
                (4, 0): 'u32', (8, 1): 's64', (8, 0): 'u64', (16, 0): 'u128'}[INTS[base]]
    base = FLOATS.get(base, base)
    if base in FNPTR and not stars:
        return FNPTR[base]
    return base + '*' * stars

def norm_params(p):
    p = ' '.join(p.split())
    if p == '':
        return None  # unspecified: `f()`
    if p == 'void':
        return ()
    return tuple('...' if a.strip() == '...' else norm_type(a) for a in split_top(p))

def knr_params(names, decls):
    """K&R definition: identifier list plus declarations before the body."""
    types = {}
    for d in decls.split(';'):
        d = d.strip()
        if not d:
            continue
        parts = split_top(d)
        first = parts[0]
        m = re.match(r'^(.*?[\s\*])\s*([A-Za-z_]\w*)\s*((?:\[[^\]]*\])*)$', first)
        if not m:
            continue
        base = m.group(1).replace('*', ' ').strip()
        types[m.group(2)] = norm_type(m.group(1) + m.group(3), drop_name=False)
        for q in parts[1:]:
            mq = re.match(r'^\s*(\**)\s*([A-Za-z_]\w*)\s*((?:\[[^\]]*\])*)$', q)
            if mq:
                types[mq.group(2)] = norm_type(base + ' ' + mq.group(1) + mq.group(3), drop_name=False)
    return tuple(types.get(n.strip(), 's32') for n in names)

# a declaration or definition head: [storage] TYPE NAME (
HEAD = re.compile(r'^([ \t]*)((?:(?:static|extern|inline|__inline)\s+)*)'
                  r'([A-Za-z_][\w \t\*]*?[\s\*])([A-Za-z_]\w*)\s*\(', re.M)
UNIT_MARK = "unit's own declaration:"
NL = chr(10)
DIRECTIVE = re.compile(r'^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b(.*)$', re.M)

class Item:
    __slots__ = ('name', 'file', 'line', 'params', 'ret', 'static', 'nm', 'block', 'module',
                 'start', 'end', 'sig', 'store', 'indent', 'praw', 'rraw', 'pend', 'knr', 'unit', 'bend')
    def __init__(self, **kw):
        for k, v in kw.items():
            setattr(self, k, v)

def nm_lines(t):
    """Set of line numbers inside `#ifdef NON_MATCHING` (the code the normal build skips)."""
    stack, out = [], set()
    lines = t.split('\n')
    for i, l in enumerate(lines, 1):
        m = re.match(r'^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b(.*)$', l)
        if m:
            k, rest = m.group(1), m.group(2)
            if k in ('if', 'ifdef', 'ifndef'):
                isnm = 'NON_MATCHING' in rest and k != 'ifndef' and '!' not in rest
                isnotnm = 'NON_MATCHING' in rest and not isnm
                stack.append([isnm, isnotnm])
            elif k in ('else', 'elif') and stack:
                a, b = stack[-1]
                stack[-1] = [b, a]
            elif k == 'endif' and stack:
                stack.pop()
            continue
        if any(s[0] for s in stack):
            out.add(i)
    return out

def scan(path, module):
    src = open(path, encoding='utf-8', errors='replace').read()
    t = strip_comments(src)
    nml = nm_lines(t)
    # brace depth at each offset, to tell block-scope declarations
    depth = []
    d = 0
    for c in t:
        depth.append(d)
        if c == '{': d += 1
        elif c == '}': d -= 1
    out = []
    for m in HEAD.finditer(t):
        name = m.group(4)
        ret = m.group(3).strip()
        if name in KW or ret.split()[0] in KW or ret.startswith('return') or '=' in ret:
            continue
        if re.search(r'\b(return|else|case|goto|do)\b', ret):
            continue
        i = m.end(); dd = 1
        while i < len(t) and dd:
            if t[i] == '(': dd += 1
            elif t[i] == ')': dd -= 1
            elif t[i] in ';{}': break
            i += 1
        if dd:
            continue
        inner = t[m.end():i - 1]
        j = i
        while j < len(t) and t[j] in ' \t\n': j += 1
        k = t[j] if j < len(t) else ''
        line = t.count('\n', 0, m.start()) + 1
        static = 'static' in m.group(2).split()
        knr = False
        blk = depth[m.start()] > 0
        if k == ';':
            if blk and m.group(2).strip() == '' and ret in ('',):
                continue
            ps = norm_params(inner)
            kind = 'decl'
        elif k == '{' and not blk:
            ps = norm_params(inner)
            if ps is None:
                ps = ()
            kind = 'def'
        elif not blk and re.match(r'^[\w\s,]*$', inner) and inner.strip():
            # K&R definition
            e = t.find('{', j)
            if e < 0 or ';' not in t[j:e] or '(' in t[j:e]:
                continue
            ps = knr_params(inner.split(','), t[j:e])
            kind = 'def'
            knr = True
        else:
            continue
        rt = norm_type(ret, drop_name=False)
        # the unit marker: on the declaration's lines or the line before them
        a0 = src.rfind(NL, 0, max(src.rfind(NL, 0, m.start()), 0)) + 1
        a1 = src.find(NL, j)
        unit = UNIT_MARK in src[a0:a1 if a1 >= 0 else len(src)]
        if knr:
            # K&R: spell the parameters out
            names = [x.strip() for x in inner.split(',')]
            pt = ', '.join((p[:-1].rstrip() + ' *' + n.lstrip('*') if False else
                             (p.rstrip('*') + ' ' + '*' * (len(p) - len(p.rstrip('*'))) + n))
                            for p, n in zip(ps, names))
            praw = pt.split(', ')
        else:
            pt = ' '.join(inner.split())
            praw = [' '.join(x.split()) for x in split_top(inner)]
        rtxt = ' '.join(t for t in ret.split() if t not in ('asm', 'inline', '__inline'))
        sig = rtxt + ('' if rtxt.endswith('*') else ' ') + name + '(' + pt + ')'
        out.append((kind, Item(name=name, file=path, line=line, params=ps, ret=rt,
                               static=static, nm=line in nml, block=blk, module=module,
                               start=m.start(), end=j + 1, sig=sig, praw=praw, rraw=rtxt, pend=i, knr=knr, unit=unit, store=m.group(2), indent=m.group(1))))
    return out

def read_typedefs(root):
    """Scalar typedef aliases (`typedef s32 Foo;`), to compare by their base type."""
    for f in glob.glob(root + '/include/**/*.h', recursive=True) + glob.glob(root + '/src/**/*.[ch]', recursive=True):
        t = strip_comments(open(f, encoding='utf-8', errors='replace').read())
        for m in re.finditer(r'^\s*#\s*define\s+(\w+)\s+(s8|u8|s16|u16|s32|u32|s64|u64|f32)\s*$', t, re.M):
            CANON[m.group(1)] = m.group(2)
        for m in re.finditer(r'^\s*typedef\s+([\w\s\*]+?)\(\s*\*\s*(\w+)\s*\)\s*\(([^;]*)\)\s*;', t, re.M):
            FNPTR.setdefault(m.group(2), None) or None
            FNPTR[m.group(2)] = FNPTR[m.group(2)] or norm_type(m.group(1) + '(*)(' + m.group(3) + ')')
        for m in re.finditer(r'^\s*typedef\s+([\w\s]+?)\s*(\*+)\s*(\w+)\s*;', t, re.M):
            CANON.setdefault(m.group(3), norm_type(m.group(1), drop_name=False) + m.group(2))
        for m in re.finditer(r'^\s*typedef\s+([\w\s]+?)\s+(\w+)\s*;', t, re.M):
            base = ' '.join(m.group(1).split())
            if base in INTS or base in FLOATS:
                CANON[m.group(2)] = base

def includes(path, root, cache):
    """Headers a file includes, transitively (quoted includes, include/ and the file's dir)."""
    if path in cache:
        return cache[path]
    cache[path] = set()
    res = set()
    try:
        src = open(path, encoding='utf-8', errors='replace').read()
    except OSError:
        return res
    for m in re.finditer(r'^\s*#\s*include\s*"([^"]+)"', src, re.M):
        for base in (os.path.dirname(path), root + '/include', root + '/src'):
            p = os.path.normpath(os.path.join(base, m.group(1)))
            if os.path.exists(p):
                res.add(p)
                res |= includes(p, root, cache)
                break
    cache[path] = res
    return res

def is_ptr(x): return x.endswith('*') or '(*)' in x
def is_int(x): return x in ('s8', 'u8', 's16', 'u16', 's32', 'u32', 's64', 'u64', 'u128') or x in CANON
def is_flt(x): return x in ('f32', 'f64')
def cls(x): return 'F' if is_flt(x) else ('.' if x == '...' else 'I')

def classify(dec, dfn):
    kinds = []
    dp, fp = dec.params, dfn.params
    if dp is None:
        if fp:
            kinds.append('unspec')
    elif len(dp) != len(fp):
        kinds.append('count')
    elif [cls(x) for x in dp] != [cls(x) for x in fp]:
        kinds.append('order')
    dr, fr = dec.ret, dfn.ret
    if dr != fr:
        if (dr == 'void') != (fr == 'void') or is_flt(dr) != is_flt(fr) or is_ptr(dr) != is_ptr(fr):
            kinds.append('return')
        elif is_ptr(dr):
            kinds.append('retptr')
        else:
            kinds.append('retw')
    if dp is not None and len(dp) == len(fp) and 'order' not in kinds:
        for x, y in zip(dp, fp):
            if x == y:
                continue
            if is_ptr(x) != is_ptr(y) and (is_int(x) or is_int(y)):
                kinds.append('ptrint')
            elif is_int(x) and is_int(y):
                kinds.append('width')
            elif is_ptr(x) and is_ptr(y):
                kinds.append('ptr')
            else:
                kinds.append('other')
    order = ['count', 'unspec', 'order', 'return', 'ptrint', 'width', 'retw', 'ptr', 'retptr', 'other']
    return sorted(set(kinds), key=order.index)

SHARED = ('main', 'sub_main')

def collect(root):
    read_typedefs(root)
    defs = collections.defaultdict(list)    # (module, name) -> [Item]
    sdefs = {}                               # (file, name) -> Item (static)
    decls = []
    cache = {}
    hdr_mods = collections.defaultdict(set)  # header -> modules including it
    for f in sorted(glob.glob(root + '/src/*/**/*.c', recursive=True)):
        mod = os.path.relpath(f, root + '/src').split(os.sep)[0]
        for h in includes(f, root, cache):
            hdr_mods[h].add(mod)
        for kind, it in scan(f, mod):
            if kind == 'def':
                if it.static:
                    sdefs.setdefault((f, it.name), it)
                else:
                    defs[(mod, it.name)].append(it)
            else:
                decls.append(it)
    hdrs = sorted(set(glob.glob(root + '/include/**/*.h', recursive=True)) |
                  set(glob.glob(root + '/src/**/*.h', recursive=True)))
    for h in hdrs:
        for kind, it in scan(h, None):
            if kind == 'decl':
                it.module = None
                decls.append(it)
    return defs, sdefs, decls, hdr_mods

def pick(lst):
    """The definition the normal build compiles (outside NON_MATCHING) if there is one."""
    for it in lst:
        if not it.nm:
            return it
    return lst[0]

def resolve(dec, defs, sdefs, hdr_mods):
    """The definitions a declaration stands for: [(Item)]."""
    if dec.module is not None:
        if (dec.file, dec.name) in sdefs:
            return [sdefs[(dec.file, dec.name)]]
        for m in (dec.module,) + SHARED:
            if m == 'main' and dec.module != 'main' and False:
                continue
            if (m, dec.name) in defs:
                return [pick(defs[(m, dec.name)])]
        return []
    mods = hdr_mods.get(os.path.normpath(dec.file), set())
    out = []
    for m in sorted(mods) + list(SHARED):
        if (m, dec.name) in defs:
            d = pick(defs[(m, dec.name)])
            if d not in out:
                out.append(d)
    # a header's static inline functions etc. are not checked
    return out

def loaded_with(root):
    '''LOADED_WITH from tools/configure.py: the modules each module links against.'''
    src = open(os.path.join(root, 'tools', 'configure.py'), encoding='utf-8').read()
    m = re.search(r'^LOADED_WITH = (\{.*?^\})', src, re.S | re.M)
    return eval(m.group(1)) if m else {}

def resolve_x(dec, defs, hdr_mods, lw):
    '''Definitions in the modules the declaration's module is loaded with (not its own,
    main or sub_main): the cross-module definitions, tried when resolve() finds none.'''
    if dec.module is not None:
        mods = {dec.module}
    else:
        mods = set(hdr_mods.get(os.path.normpath(dec.file), set()))
    cand = sorted({x for m in mods for x in lw.get(m, [])} - mods - set(SHARED))
    out = []
    for m in cand:
        if (m, dec.name) in defs:
            out.append(pick(defs[(m, dec.name)]))
    return out

def fmt(ps):
    return '()' if ps is None else '(' + ', '.join(ps or ('void',)) + ')'

def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('repo', nargs='?', default='.')
    ap.add_argument('--list', help='kinds to list (comma-separated, or "all")')
    ap.add_argument('--module', help='only declarations in this module (headers: "include")')
    ap.add_argument('--file', help='only declarations in files whose path contains this')
    ap.add_argument('--callee', help='only declarations of this function')
    ap.add_argument('--by-module', action='store_true', help='summary per module and kind')
    ap.add_argument('--primary', action='store_true', help='--list matches the first kind only')
    a = ap.parse_args()
    root = os.path.abspath(a.repo)
    defs, sdefs, decls, hdr_mods = collect(root)
    lw = loaded_with(root)
    rows = []
    for dec in decls:
        modname = dec.module or 'include'
        if a.module and modname != a.module:
            continue
        rel = os.path.relpath(dec.file, root)
        if a.file and a.file not in rel:
            continue
        if a.callee and dec.name != a.callee:
            continue
        dfns = resolve(dec, defs, sdefs, hdr_mods)
        if dfns:
            for dfn in dfns:
                ks = classify(dec, dfn)
                if ks and dec.unit:
                    ks = ['unit'] + ks
                if ks:
                    rows.append((ks, modname, rel, dec, dfn))
            continue
        # across modules: a disagreement only if no candidate definition agrees
        xs = [(classify(dec, dfn), dfn) for dfn in resolve_x(dec, defs, hdr_mods, lw)]
        if xs and all(ks for ks, _ in xs):
            ks, dfn = xs[0]
            ks = (['unit'] + ks) if dec.unit else ks
            rows.append((['x' + k for k in ks], modname, rel, dec, dfn))
    order = ['count', 'unspec', 'order', 'return', 'ptrint', 'width', 'retw', 'ptr', 'retptr',
             'other', 'unit']
    order += ['x' + k for k in order]
    if a.list:
        want = set(a.list.split(','))
        if 'all' in want:
            want |= {k for k in order if not k.startswith('x')}
        if 'cross' in want:
            want |= {k for k in order if k.startswith('x')}
        for ks, modname, rel, dec, dfn in rows:
            if ks[0] in ('unit', 'xunit') and ks[0] in want or ks[0] not in ('unit', 'xunit') and (
                    (ks[0] in want) if a.primary else (want & set(ks))):
                print(f'{",".join(ks)} {dec.name}: {rel}:{dec.line}{" [block]" if dec.block else ""}'
                      f'{" [NM]" if dec.nm else ""} {dec.ret} {fmt(dec.params)} | def '
                      f'{os.path.relpath(dfn.file, root)}:{dfn.line} {dfn.ret} {fmt(dfn.params)}')
        return
    c = collections.Counter(r[0][0] for r in rows)
    callees = collections.defaultdict(set)
    for ks, modname, rel, dec, dfn in rows:
        callees[ks[0]].add(dec.name)
    for k in (k for k in order if c[k] or not k.startswith('x')):
        print(f'{k:7} {c[k]:5} declarations, {len(callees[k]):4} functions')
    nu = c['unit'] + c['xunit']
    nx = sum(v for k, v in c.items() if k.startswith('x') and k != 'xunit')
    print(f'total   {len(rows) - nu:5} disagreements ({nx} across modules), plus {nu} marked '
          f'unit ({len(decls)} declarations checked)')
    if a.by_module:
        bm = collections.Counter((r[1], r[0][0]) for r in rows)
        for m in sorted({r[1] for r in rows}):
            print(f'{m:9} ' + ' '.join(f'{k}={bm[(m, k)]}' for k in order if bm[(m, k)]))

if __name__ == '__main__':
    main()

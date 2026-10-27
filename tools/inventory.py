#!/usr/bin/env python3
"""The public surface of GNU FriBidi 1.0.16, from the pinned release.

Shared by tools/gen_exports.py (which writes the module sources and the
generated macro test) and tools/check_exports.py (which audits them). The
headers come from the upstream tarball, downloaded once into target/upstream
and checked against upstream/documents.json: the same bytes compat.fribidi
builds.

The public headers are the ones upstream installs (lib/meson.build,
`fribidi_headers`), plus the two it generates and installs beside them:
fribidi-config.h (from fribidi-config.h.in) and fribidi-unicode-version.h.
The per-charset headers (fribidi-char-sets-utf8.h …) are not installed and
their functions are not FRIBIDI_ENTRY: they are internal.

The headers are read as a C++ translation unit sees them: the branch of
fribidi-bidi-types.h that declares FriBidiCharType / FriBidiParType as enums
is C only (`!defined(__cplusplus)`); in C++ they are uint32_t and the
FRIBIDI_TYPE_* / FRIBIDI_PAR_* names are macros. `__FRIBIDI_DOC` branches are
documentation only.
"""
from dataclasses import dataclass, field
import hashlib
import json
from pathlib import Path
import re
import sys
import tarfile
import urllib.request

INSTALLED = [  # lib/meson.build: fribidi_headers, in fribidi.h's include order where it has one
    'fribidi-common.h', 'fribidi-unicode.h', 'fribidi-types.h', 'fribidi-flags.h',
    'fribidi-bidi-types.h', 'fribidi-bidi.h', 'fribidi-joining-types.h', 'fribidi-joining.h',
    'fribidi-mirroring.h', 'fribidi-brackets.h', 'fribidi-arabic.h', 'fribidi-shape.h',
    'fribidi-char-sets.h', 'fribidi-deprecated.h', 'fribidi.h',
    'fribidi-begindecls.h', 'fribidi-enddecls.h',
    'fribidi-bidi-types-list.h', 'fribidi-joining-types-list.h', 'fribidi-char-sets-list.h',
]
GENERATED = ['fribidi-config.h.in', 'fribidi-unicode-version.h']


def fetch(root: Path) -> Path:
    pins = json.loads((root / 'upstream/documents.json').read_text(encoding='utf-8'))
    cache = root / 'target/upstream'
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / Path(pins['upstream']).name
    if not archive.exists() or hashlib.sha256(archive.read_bytes()).hexdigest() != pins['upstream_sha256']:
        print('+ fetch', pins['upstream'], flush=True)
        with urllib.request.urlopen(pins['upstream'], timeout=120) as response:
            archive.write_bytes(response.read())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != pins['upstream_sha256']:
        sys.exit(f'upstream archive checksum {digest} != {pins["upstream_sha256"]}')
    with tarfile.open(archive) as tar:
        top = tar.getmembers()[0].name.split('/')[0]
        if not (cache / top / 'lib/fribidi.h').exists():
            tar.extractall(cache, filter='data')
    return cache / top


@dataclass
class Macro:
    name: str
    header: str
    params: list[str] | None   # None: object-like
    body: str


@dataclass
class Surface:
    types: list[str] = field(default_factory=list)
    enumerators: list[str] = field(default_factory=list)
    functions: list[str] = field(default_factory=list)
    cxx_linkage: list[str] = field(default_factory=list)   # declared outside FRIBIDI_BEGIN_DECLS: C++ linkage
    signatures: dict[str, tuple[str, str]] = field(default_factory=dict)  # function: (return type, parameters)
    variables: list[str] = field(default_factory=list)
    macros: dict[str, Macro] = field(default_factory=dict)


def strip(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def cxx_view(name: str, text: str) -> str:
    """The text a C++ translation unit compiles: C-only and doc-only branches cut."""
    if name == 'fribidi-bidi-types.h':
        start = text.index('#if defined(__FRIBIDI_DOC) || (FRIBIDI_SIZEOF_INT+0 == 4 && !defined(__cplusplus))')
        middle = text.index('#else', start)
        text = text[:start] + text[middle + len('#else'):]
    if name == 'fribidi-joining-types.h':
        start = text.index('#ifdef __FRIBIDI_DOC')
        middle = text.index('#else /* !__FRIBIDI_DOC */', start)
        text = text[:start] + text[middle + len('#else /* !__FRIBIDI_DOC */'):]
    return text


def x_entries(lib: Path, list_header: str, adder: str) -> list[str]:
    """The first arguments of an X-macro list's entries for `adder`."""
    text = strip((lib / list_header).read_text(encoding='utf-8'))
    aliases = {adder}
    if adder == '_FRIBIDI_ADD_CHAR_SET':  # the list defines these two as it when it is defined
        aliases |= {'_FRIBIDI_ADD_CHAR_SET_OTHERS', '_FRIBIDI_ADD_CHAR_SET_ONE2ONE'}
    if list_header == 'fribidi-bidi-types-list.h':
        sys.exit('fribidi-bidi-types-list.h builds no C++ declaration')
    names = []
    for line in text.splitlines():
        m = re.match(r'\s*(_FRIBIDI_ADD_\w+)\s*\(\s*(\w+)\s*,', line)
        if m and m.group(1) in aliases:
            names.append(m.group(2))
    return names


def enumerators(lib: Path, body: str) -> list[str]:
    """The enumerators of an enum body, its X-macro inclusions expanded."""
    out = []
    adder = None
    for line in body.replace('\\\n', ' ').splitlines():
        d = re.match(r'\s*#\s*define\s+(_FRIBIDI_ADD_\w+)\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)\s*(.*)', line)
        if d:
            adder = (d.group(1), d.group(2), d.group(4).strip())
            continue
        inc = re.match(r'\s*#\s*include\s+"([^"]+)"', line)
        if inc:
            name, param, template = adder
            m = re.fullmatch(r'(\w*?)##' + param + r'(?:##(\w+))?\s*(?:=.*)?,', template)
            if not m:
                sys.exit(f'unexpected X-macro expansion {template!r}')
            out += [m.group(1) + entry + (m.group(2) or '') for entry in x_entries(lib, inc.group(1), name)]
            continue
        if line.lstrip().startswith('#'):
            continue
        out += re.findall(r'\b([A-Za-z_]\w*)\b\s*(?:=[^,]*)?,?', line)
    return out


def surface(upstream: Path) -> Surface:
    lib = upstream / 'lib'
    s = Surface()
    types, enums, functions, variables = [], [], [], []
    for name in INSTALLED + GENERATED:
        raw = (lib / name).read_text(encoding='utf-8')
        for m in re.finditer(r'^[ \t]*#[ \t]*define[ \t]+(\w+)(\(([^)]*)\))?[ \t]*((?:[^\n]*\\\n)*[^\n]*)', strip(raw), flags=re.M):
            params = [p.strip() for p in m.group(3).split(',')] if m.group(2) else None
            body = ' '.join(m.group(4).replace('\\\n', ' ').split())
            s.macros.setdefault(m.group(1), Macro(m.group(1), name, params, body))
        if name in GENERATED or name.endswith('-list.h'):
            continue
        text = strip(cxx_view(name, raw))
        # Named and typedef'd enums, their X-macro bodies expanded.
        for m in re.finditer(r'\b(typedef\s+)?enum\s*(\w*)\s*\{(.*?)\}\s*(\w*)\s*;', text, flags=re.S):
            for type_name in (m.group(2), m.group(4)):
                if type_name:
                    types.append(type_name)
            enums += enumerators(lib, m.group(3))
        # Declarations only: the preprocessor's lines are not.
        code = re.sub(r'^[ \t]*#(?:[^\n]*\\\n)*[^\n]*', '', text, flags=re.M)
        code = re.sub(r'\benum\s*\w*\s*\{.*?\}\s*\w*\s*;', '', code, flags=re.S)
        types += re.findall(r'\btypedef\s+[\w\s\*]+?\b(\w+)\s*;', code)
        # Attribute macros hide a declaration from a naive match: normalise
        # them away first (FRIBIDI_GNUC_CONST, FRIBIDI_GNUC_WARN_UNUSED … trail
        # the parameter list; FRIBIDI_EXTERN precedes a variable).
        plain = re.sub(r'\bFRIBIDI_GNUC_\w+|\bFRIBIDI_EXTERN\b', ' ', code)
        # A header without fribidi-begindecls.h declares in C++ linkage what
        # the library defines in C linkage (fribidi-common.h).
        c_block = '#include "fribidi-begindecls.h"' in raw
        for m in re.finditer(r'\bFRIBIDI_ENTRY\b([^;{]*);', plain):
            declaration = ' '.join(m.group(1).split())
            f = re.search(r'\b(\w+)\s*\(', declaration)
            if f:
                functions.append(f.group(1))
                sig = re.fullmatch(r'(.*?)\b' + f.group(1) + r'\s*\((.*)\)', declaration)
                s.signatures[f.group(1)] = (sig.group(1).strip(), ' '.join(sig.group(2).split()))
                if not c_block:
                    s.cxx_linkage.append(f.group(1))
            else:
                variables.append(re.findall(r'(\w+)\s*$', declaration)[0])
        # Everything that is not FRIBIDI_ENTRY must not be a declaration of a function.
        unannotated = re.sub(r'\bFRIBIDI_ENTRY\b[^;{]*;', ';', plain)
        unannotated = re.sub(r'\btypedef\b[^;]*;', ';', unannotated)
        stray = re.findall(r'\b(\w+)\s*\([^;{)]*\)\s*;', unannotated)
        if stray:
            sys.exit(f'{name}: declarations without FRIBIDI_ENTRY: {stray}')
    s.types = sorted(set(types))
    s.enumerators = list(dict.fromkeys(enums))
    s.functions = sorted(set(functions))
    s.variables = sorted(set(variables))
    return s


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    sf = surface(fetch(root))
    print(f'{len(sf.types)} types, {len(sf.enumerators)} enumerators, {len(sf.functions)} functions, '
          f'{len(sf.variables)} variables, {len(sf.macros)} macros')
    print('types:', ' '.join(sf.types))
    print('enumerators:', ' '.join(sf.enumerators))
    print('functions:', ' '.join(sf.functions))
    print('declared in C++ linkage:', ' '.join(sf.cxx_linkage))
    print('variables:', ' '.join(sf.variables))
    print('function-like:', ' '.join(m.name for m in sf.macros.values() if m.params is not None))

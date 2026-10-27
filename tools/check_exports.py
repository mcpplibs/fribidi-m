#!/usr/bin/env python3
"""Audit the module surface against the pinned release's public FriBidi headers.

- src/fribidi.cppm, src/common.cpp, src/macros.cppm and
  tests/macros_mix_test.cpp are what tools/gen_exports.py writes.
- Every type, enumerator, FRIBIDI_ENTRY function and variable of the
  installed headers is exported by `export using`, except a function the
  headers declare in C++ linkage (outside FRIBIDI_BEGIN_DECLS), which cannot
  link: that one is adapted under its name in mcpplibs::mwrap::fribidi.
- Every macro of the headers is mapped in src/macros.cppm (a constant or a
  function), kept as a macro in <mwrap/fribidi/macros.hpp> with its reason,
  or an include guard; and each mapping is checked against the raw header by
  tests/macros_mix_test.cpp.
"""
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import fetch, surface  # noqa: E402
from gen_exports import SIDE_HEADER, excluded  # noqa: E402


def mapped_names(text: str) -> set[str]:
    text = text[text.index('export module'):]
    names = set(re.findall(r'^inline constexpr auto&? (\w+) = c::value_\1;', text, flags=re.M))
    names |= set(re.findall(r'^constexpr (?:auto|decltype\(auto\)) \((\w+)\)\(', text, flags=re.M))
    return names


def main():
    root = Path(__file__).resolve().parents[1]
    problems = []
    generated = subprocess.run([sys.executable, str(root / 'tools/gen_exports.py'), '--check'], capture_output=True, text=True)
    if generated.returncode != 0:
        problems.append(generated.stderr.strip() or generated.stdout.strip())
    sf = surface(fetch(root))
    module = (root / 'src/fribidi.cppm').read_text(encoding='utf-8')
    exported = set(re.findall(r'^export using ::(\w+);', module, flags=re.M))
    declared = set(sf.types) | set(sf.enumerators) | set(sf.functions) | set(sf.variables)
    missing = declared - exported - set(sf.cxx_linkage)
    if missing:
        problems.append('src/fribidi.cppm lacks: ' + ', '.join(sorted(missing)))
    extra = exported - declared
    if extra:
        problems.append('src/fribidi.cppm exports what the headers do not declare: ' + ', '.join(sorted(extra)))
    adapted = set(re.findall(r'^extern "C\+\+" [\w\s\*]+?\b(\w+)\(', module, flags=re.M))
    if adapted != set(sf.cxx_linkage):
        problems.append(f'adapted {sorted(adapted)} != declared in C++ linkage {sorted(sf.cxx_linkage)}')
    if exported & adapted:
        problems.append('both exported and adapted: ' + ', '.join(sorted(exported & adapted)))
    common = (root / 'src/common.cpp').read_text(encoding='utf-8')
    for name in sf.cxx_linkage:
        if not re.search(r'\b' + name + r'\([^)]*\) \{ return ::' + name + r'\(', common):
            problems.append(f'src/common.cpp does not adapt {name}')

    mapped = mapped_names((root / 'src/macros.cppm').read_text(encoding='utf-8'))
    macros = set(sf.macros)
    guards = {m for m in macros if excluded(m)}
    unaccounted = macros - mapped - SIDE_HEADER.keys() - guards
    if unaccounted:
        problems.append('macros neither mapped, side-header nor include guards: ' + ', '.join(sorted(unaccounted)))
    both = SIDE_HEADER.keys() & mapped
    if both:
        problems.append('listed as side-header yet mapped: ' + ', '.join(sorted(both)))
    stale = SIDE_HEADER.keys() - macros
    if stale:
        problems.append('listed but not in the headers: ' + ', '.join(sorted(stale)))
    test = (root / 'tests/macros_mix_test.cpp').read_text(encoding='utf-8')
    untested = {n for n in mapped if f'm::{n}' not in test}
    if untested:
        problems.append('mapped but not checked against the header: ' + ', '.join(sorted(untested)))
    side = (root / 'include/mwrap/fribidi/macros.hpp').read_text(encoding='utf-8')
    if '#include <fribidi.h>' not in side:
        problems.append('<mwrap/fribidi/macros.hpp> does not include <fribidi.h>')
    if problems:
        sys.exit('\n'.join(problems))
    functions = sum(1 for m in sf.macros.values() if m.name in mapped and m.params is not None)
    print(f'{len(sf.types)} types, {len(sf.enumerators)} enumerators, {len(sf.functions) - len(sf.cxx_linkage)} functions '
          f'and {len(sf.variables)} variables exported; {len(sf.cxx_linkage)} functions declared in C++ linkage adapted '
          f'({", ".join(sf.cxx_linkage)}); {len(macros)} macros: {len(mapped) - functions} constants and {functions} '
          f'functions mapped (each checked against the header), {len(SIDE_HEADER)} kept as macros, {len(guards)} include guards')


if __name__ == '__main__':
    main()

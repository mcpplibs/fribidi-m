#!/usr/bin/env python3
"""Run GNU FriBidi's own test suite (test/ of the 1.0.16 release, every test
upstream's meson.build and run.tests define) on examples/fribidi, programs
written against import mcpplibs.mwrap.fribidi.

The release tarball is downloaded once into target/upstream and checked
against upstream/documents.json; its files are used unchanged.

- The character-set tests, test/test_<CHARSET>_<name>.input: the output of
  `fribidi --test --charset CHARSET <input>` must equal <reference> byte for
  byte (CRLF read as LF on Windows, as upstream's test-runner.py does). The
  fribidi program is examples/fribidi/src/main.cpp, upstream's
  bin/fribidi-main.c ported to the module.
- The Unicode conformance tests: BidiTest.txt (every line under each
  paragraph direction it lists) and BidiCharacterTest.txt, through ports of
  upstream's runners (examples/fribidi/conformance/).

Nothing is excluded: the release ships no fuzzing corpus (upstream runs it
only with -Dfuzzer_ldflags). Every test must pass.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import fetch  # noqa: E402

# The release's counts: a guard against reading nothing.
BIDI_TESTS = 770241
BIDI_CHARACTER_TESTS = 91707
CHARSET_TESTS = 6


def build(example: Path, mcpp: str, toolchain: str | None) -> Path:
    command = [mcpp, 'build', '--profile', 'release'] + (['--toolchain', toolchain] if toolchain else [])
    print('+', ' '.join(command), flush=True)
    subprocess.run(command, cwd=example, check=True)
    suffix = '.exe' if os.name == 'nt' else ''
    found = sorted((p for p in (example / 'target').rglob('fribidi' + suffix) if p.parent.name == 'bin'),
                   key=lambda p: p.stat().st_mtime)
    if not found:
        sys.exit('examples/fribidi was not built')
    return found[-1].parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcpp', default='mcpp')
    parser.add_argument('--toolchain')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    upstream = fetch(root)
    bin_dir = build(root / 'examples/fribidi', args.mcpp, args.toolchain)
    exe = lambda name: str(bin_dir / (name + ('.exe' if os.name == 'nt' else '')))  # noqa: E731

    failed = []
    charset_tests = sorted((upstream / 'test').glob('test_*.input'))
    for test in charset_tests:
        charset = re.fullmatch(r'test_(.+)_[^_]+\.input', test.name).group(1)
        reference = test.with_suffix('.reference').read_bytes().replace(b'\r\n', b'\n')
        run = subprocess.run([exe('fribidi'), '--test', '--charset', charset, str(test)], capture_output=True, timeout=300)
        output = run.stdout.replace(b'\r\n', b'\n')
        ok = run.returncode == 0 and output == reference
        print(f'{"ok  " if ok else "FAIL"} fribidi --test --charset {charset} {test.name}', flush=True)
        if not ok:
            failed.append(test.name)
            print(f'exit {run.returncode}\n--- output\n{output.decode("utf-8", "replace")}\n--- reference\n'
                  f'{reference.decode("utf-8", "replace")}\n{run.stderr.decode("utf-8", "replace")}', flush=True)

    counts = {}
    for runner, data in (('BidiTest', 'BidiTest.txt'), ('BidiCharacterTest', 'BidiCharacterTest.txt')):
        run = subprocess.run([exe(runner), str(upstream / 'test/unicode-conformance' / data)], capture_output=True,
                             text=True, timeout=1800)
        summary = re.search(runner + r': (\d+) tests, (\d+) errors', run.stdout)
        counts[runner] = int(summary.group(1)) if summary else 0
        ok = run.returncode == 0 and summary is not None and summary.group(2) == '0'
        print(f'{"ok  " if ok else "FAIL"} {runner} {data}: {summary.group(0) if summary else "no summary"}', flush=True)
        if not ok:
            failed.append(data)
            print(run.stderr[-4000:], flush=True)

    print(f'{len(charset_tests)} character-set tests; BidiTest.txt {counts["BidiTest"]} tests; '
          f'BidiCharacterTest.txt {counts["BidiCharacterTest"]} tests; {len(failed)} failed')
    if failed or len(charset_tests) != CHARSET_TESTS or counts['BidiTest'] != BIDI_TESTS \
            or counts['BidiCharacterTest'] != BIDI_CHARACTER_TESTS:
        sys.exit(1)


if __name__ == '__main__':
    main()

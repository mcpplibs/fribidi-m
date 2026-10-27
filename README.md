# fribidi

> GNU FriBidi, the Unicode Bidirectional Algorithm, through C++23 modules.

**English** · [简体中文](README.zh.md) · [Upstream](https://github.com/fribidi/fribidi) · [Releases](https://github.com/mcpplibs/fribidi-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![FriBidi](https://img.shields.io/badge/FriBidi-LGPL--2.1--or--later-blue.svg)](upstream/fribidi/COPYING)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/fribidi-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/fribidi-m/actions/workflows/ci.yml)

FriBidi turns text stored in logical order into the visual order a display
needs for Arabic, Hebrew and mixed-direction text (UAX #9, Unicode 16.0.0).
The upstream C API is unchanged: `FriBidiCharType`,
`fribidi_get_par_embedding_levels_ex`, `fribidi_reorder_line` and
`fribidi_log2vis` are the upstream globals of `<fribidi.h>`. The library is
`compat.fribidi` 1.0.16. This package maps FriBidi's macros to constants and
constexpr functions, each checked against the header. FriBidi's own test
suite passes on every CI run: the charset tests and all 861,948 Unicode
conformance cases, on programs written against the module.

## Modules

- `import mcpplibs.mwrap.fribidi;`: the whole API of `<fribidi.h>`, re-exporting the module below
- `import mcpplibs.mwrap.fribidi.macros;`: `FRIBIDI_PAR_ON`, `FRIBIDI_TYPE_RTL`, `FRIBIDI_FLAGS_DEFAULT`, `FRIBIDI_IS_RTL` … in `mcpplibs::mwrap::fribidi::macros`

> More Modules API -> [Usage and API](docs/usage.md)

## Getting started

```sh
mcpp new bidi --template mcpplibs.mwrap.fribidi@1.0.16
cd bidi
mcpp run
```

The default template lays out UTF-8 lines as a renderer does: it computes
bidi types, bracket pairs and embedding levels, then reorders each line to
visual order with mirrored brackets. Run `mcpp run -- "your text"` for your
own lines.

```toml
[dependencies]
"mcpplibs.mwrap.fribidi" = "1.0.16"
```

```cpp
import std;
import mcpplibs.mwrap.fribidi;
using namespace mcpplibs::mwrap::fribidi::macros;  // FRIBIDI_PAR_ON, FRIBIDI_LEVEL_IS_RTL …

int main() {
    std::u32string_view text = U"abc אבג (123)";
    std::vector<FriBidiChar> str(text.begin(), text.end());
    auto n = static_cast<FriBidiStrIndex>(str.size());
    std::vector<FriBidiCharType> types(str.size());
    std::vector<FriBidiBracketType> brackets(str.size());
    std::vector<FriBidiLevel> levels(str.size());
    fribidi_get_bidi_types(str.data(), n, types.data());
    fribidi_get_bracket_types(str.data(), n, types.data(), brackets.data());
    FriBidiParType base = FRIBIDI_PAR_ON;
    if (!fribidi_get_par_embedding_levels_ex(types.data(), brackets.data(), n, &base, levels.data())) return 1;
    for (FriBidiLevel level : levels) std::print("{}", FRIBIDI_LEVEL_IS_RTL(level) ? 'R' : 'L');
    std::println();
}
```

## Macros and two adapted functions

Constants such as `FRIBIDI_TYPE_LTR` keep the type their macro has in C++
(`long`). Computations such as `FRIBIDI_IS_RTL` and `FRIBIDI_SET_BITS` are
constexpr functions whose body is the macro itself. They match the macro's
value, type and value category for every argument type. Attributes,
`extern "C"` brackets, pragmas and the X-macro tables stay macros in
`<mwrap/fribidi/macros.hpp>`, included before the import.

`fribidi-common.h` declares `fribidi_debug_status` and `fribidi_set_debug`
outside its `extern "C"` block, so C++ calls to them do not link. The module
adapts both in `mcpplibs::mwrap::fribidi`. Details:
[docs/usage.md](docs/usage.md).

## Examples

- [examples/fribidi](examples/fribidi): upstream's `fribidi` command line program (logical to visual, in every
  character set it supports) and its Unicode conformance runners, written against the module.
  [tools/run_conformance.py](tools/run_conformance.py) runs FriBidi's tests on them.

## Build and testing

Linux, macOS and Windows CI build the modules, then run the tests and the
export audit ([tools/check_exports.py](tools/check_exports.py)). The audit
checks that the generated sources are fresh, that every declaration is
exported and that every macro is mapped or listed with its reason. CI also
runs FriBidi's test suite: the six charset tests, BidiTest.txt (770,241
cases) and BidiCharacterTest.txt (91,707 cases). A separate job scaffolds the
default template as an installed package and runs it.

## License

[Apache-2.0](LICENSE) for the wrapper, by mcpp-community ([NOTICE](NOTICE)).
GNU FriBidi remains LGPL-2.1-or-later
([upstream/fribidi/COPYING](upstream/fribidi/COPYING),
[authors](upstream/fribidi/AUTHORS)). It is linked statically, so a program
distributed in binary form must meet the LGPL's terms for FriBidi, including
relinking ([docs/usage.md](docs/usage.md#licensing)). Its tables come from
the Unicode Character Database
([notice](upstream/fribidi/gen.tab/unidata/ReadMe.txt)). The ports in
[examples/fribidi](examples/fribidi) remain LGPL-2.1-or-later.

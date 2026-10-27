# Using mcpplibs.mwrap.fribidi

`import mcpplibs.mwrap.fribidi;` declares GNU FriBidi 1.0.16's API as `<fribidi.h>` does. Types, enumerators, functions and variables are the upstream globals, the very same entities as the header's, so a translation unit may include the header too. The library is `compat.fribidi` 1.0.16, built from the release tarball with its Unicode 16.0.0 tables.

## Modules

| Module | What it holds |
|---|---|
| `mcpplibs.mwrap.fribidi` | every type (`FriBidiChar`, `FriBidiCharType`, `FriBidiParType`, `FriBidiLevel`, `FriBidiStrIndex`, `FriBidiBracketType`, `FriBidiJoiningType`, `FriBidiArabicProp`, `FriBidiFlags`, `FriBidiCharSet` …), the enumerators of `FriBidiCharSet` and of the joining types, every `FRIBIDI_ENTRY` function (32, the deprecated ones included) and the variables `fribidi_version_info` / `fribidi_unicode_version`; the two functions below, adapted in `mcpplibs::mwrap::fribidi`; re-exports the module below |
| `mcpplibs.mwrap.fribidi.macros` | the macros, in `mcpplibs::mwrap::fribidi::macros` |

The public headers are the ones upstream installs (`lib/meson.build`), with the two it generates (`fribidi-config.h`, `fribidi-unicode-version.h`). The per-charset headers (`fribidi-char-sets-utf8.h` …) are internal: upstream does not install them and their functions are not entry points. Convert with `fribidi_charset_to_unicode` / `fribidi_unicode_to_charset`.

## C++ types of the bidi classes

In C++, `<fribidi.h>` declares `FriBidiCharType` and `FriBidiParType` as `uint32_t`, and `FRIBIDI_TYPE_*` / `FRIBIDI_PAR_*` as macros. The enums FriBidi documents exist only in C (`!defined(__cplusplus)`). The module follows the header as C++ compiles it.

## Two functions adapted

`fribidi-common.h` declares `fribidi_debug_status` and `fribidi_set_debug` outside the `extern "C"` block every other header has. A C++ program that includes the header therefore declares them in C++ linkage, and a call does not link: the library defines them in C. The module adapts both under their names and signatures in `mcpplibs::mwrap::fribidi`. Each calls the library's C function ([src/common.cpp](../src/common.cpp)). The library is built without `DEBUG`, so both return 0.

```cpp
mcpplibs::mwrap::fribidi::fribidi_set_debug(true);  // links; returns false (no DEBUG)
```

## Macros

A named module cannot export a macro. `mcpplibs::mwrap::fribidi::macros` holds them under their original names; `using namespace mcpplibs::mwrap::fribidi::macros;` makes code read as it does with the header.

- **Constants** as `inline constexpr`, each of the type its macro expands to in C++: `FRIBIDI_TYPE_*`, `FRIBIDI_PAR_*` and `FRIBIDI_MASK_*` are `long` (`0x00000010L` …); `FRIBIDI_FLAG_*`, `FRIBIDI_FLAGS_*`, `FRIBIDI_CHAR_*`, `FRIBIDI_JOINING_TYPE_*_VAL` and `FRIBIDI_BIDI_*` are `int`; `FRIBIDI_BRACKET_OPEN_MASK` is `unsigned`; `FRIBIDI_CHAR_SET_NOT_FOUND` is a `FriBidiCharSet`. The version macros (`FRIBIDI_VERSION`, `FRIBIDI_UNICODE_VERSION`, `FRIBIDI_NAME` …) are references to constexpr char arrays. The deprecated aliases (`UNI_*`, `FRIBIDI_TYPE_L` …) are there too.
- **Computations** as `constexpr` function templates: `FRIBIDI_IS_*`, `FRIBIDI_LEVEL_IS_RTL`, `FRIBIDI_LEVEL_TO_DIR`, `FRIBIDI_DIR_TO_LEVEL`, `FRIBIDI_CHANGE_NUMBER_TO_RTL`, `FRIBIDI_WEAK_PARAGRAPH`, `FRIBIDI_JOINS_*`, `FRIBIDI_ARAB_SHAPES`, `FRIBIDI_IS_JOINING_TYPE_*`, `FRIBIDI_IS_BRACKET_OPEN`, `FRIBIDI_BRACKET_ID`, `FRIBIDI_TEST_BITS` … Each body is the macro itself, expanded on the parameters. The result has the macro's value, type and value category for every argument type.
- **Arguments.** Each argument is evaluated once, before the call. A macro may evaluate an argument more than once (`FRIBIDI_CHANGE_NUMBER_TO_RTL`, `FRIBIDI_EXPLICIT_TO_OVERRIDE_DIR`, `FRIBIDI_ADJUST_BITS`, `FRIBIDI_ADJUST_AND_TEST_BITS`); for an argument without side effects the two agree. `FRIBIDI_IS_BRACKET_OPEN(bt)` and `FRIBIDI_BRACKET_ID(bt)` do not parenthesise `bt`; the functions behave as if they did.
- **Assignments.** `FRIBIDI_SET_BITS`, `FRIBIDI_UNSET_BITS`, `FRIBIDI_ADJUST_BITS` and `FRIBIDI_ADJUST_AND_TEST_BITS` assign to their first argument. The functions take it by reference and return what the macro does: the first three return the argument itself (an lvalue), the last an `int`. A bit-field cannot bind to a reference; use the macro for one.
- **Upstream's precedence.** `FRIBIDI_IS_LTR_LETTER(p)` is `(p) & (… == …)`: `==` binds tighter than `&`, so the macro is always 0, and `FRIBIDI_IS_RTL_LETTER(p)` is `p & 1`. The functions keep these results. Use `FRIBIDI_IS_LETTER` with `FRIBIDI_IS_RTL` for the intended test.

Each mapping is checked against the raw header for every sample argument type (bidi types, masks, flags, all 256 joining properties, all levels), in [tests/macros_mix_test.cpp](../tests/macros_mix_test.cpp).

### What stays a macro

These stay macros in `<mwrap/fribidi/macros.hpp>`, which is `<fribidi.h>`:

| Macros | Why |
|---|---|
| `FRIBIDI_ENTRY`, `FRIBIDI_EXTERN`, `FRIBIDI_GNUC_CONST`, `FRIBIDI_GNUC_DEPRECATED`, `FRIBIDI_GNUC_HIDDEN`, `FRIBIDI_GNUC_MALLOC`, `FRIBIDI_GNUC_WARN_UNUSED` | attributes and storage classes of declarations |
| `FRIBIDI_BEGIN_DECLS`, `FRIBIDI_END_DECLS` | open and close an `extern "C"` block |
| `FRIBIDI_BEGIN_IGNORE_DEPRECATIONS`, `FRIBIDI_END_IGNORE_DEPRECATIONS` | diagnostic pragmas |
| `_FRIBIDI_ADD_TYPE`, `_FRIBIDI_ADD_ALIAS`, `_FRIBIDI_ADD_CHAR_SET`, `_FRIBIDI_ADD_CHAR_SET_OTHERS`, `_FRIBIDI_ADD_CHAR_SET_ONE2ONE`, `_FRIBIDI_PAR_TYPES` | the X-macro protocol of `fribidi-bidi-types-list.h`, `fribidi-joining-types-list.h` and `fribidi-char-sets-list.h`: the includer defines them |

Include the side header before the import. It defines `FRIBIDI_LIB_STATIC` on Windows, as the module does: `compat.fribidi` is a static library, so its entry points have no `dllimport`. Include it rather than `<fribidi.h>`, so the header's declarations match the module's on Windows.

While a header macro is defined, it expands even inside a qualified name. An object-like macro therefore hides `macros::FRIBIDI_PAR_RTL`; use the plain macro, or set it aside with `#pragma push_macro`. A function-like macro does not expand when its name is followed by `)`, so `(macros::FRIBIDI_IS_RTL)(t)` calls the function beside the header ([tests/header_mix_test.cpp](../tests/header_mix_test.cpp)). [tools/check_exports.py](../tools/check_exports.py) accounts for all 259 macros.

## Licensing

The wrapper's code is Apache-2.0. GNU FriBidi is LGPL-2.1-or-later ([upstream/fribidi/COPYING](../upstream/fribidi/COPYING)). Its tables are generated from the Unicode Character Database ([notice](../upstream/fribidi/gen.tab/unidata/ReadMe.txt)).

`compat.fribidi` is a static library, so a program built with this package contains FriBidi. Distributing such a program in binary form brings the LGPL-2.1 obligations for FriBidi (section 6), among them:

- say that the program uses FriBidi under the LGPL, and ship the license;
- ship FriBidi's source, or a written offer for it. The exact source is the pinned release tarball in [upstream/documents.json](../upstream/documents.json), built unmodified;
- let users relink the program with a modified FriBidi: provide the program's object files or source (section 6a), because the library is linked statically;
- do not forbid reverse engineering for debugging such modifications.

`runtime.deploy` copies the wrapper's LICENSE and NOTICE and FriBidi's README, COPYING, AUTHORS and Unicode data notice into `licenses/fribidi/` beside every program that depends on this package. The other obligations are the distributor's.

## Checks

- `mcpp test` runs four tests:
  - [tests/api_test.cpp](../tests/api_test.cpp): the API through the module only — bidi and joining types, paragraph direction and levels (UAX #9 N0's bracket example), line reordering, `fribidi_log2vis` with its maps and mirroring, bidi-mark removal, Arabic joining and shaping (presentation forms, the lam-alef ligature), the character sets, the deprecated API, the versions;
  - [tests/macros_mix_test.cpp](../tests/macros_mix_test.cpp) (generated): every constant and function against the header;
  - [tests/header_mix_test.cpp](../tests/header_mix_test.cpp): the header beside the import — the X-macro tables, the pragmas, `FRIBIDI_BEGIN_DECLS`;
  - [tests/macros_test.cpp](../tests/macros_test.cpp): the macros module alone, at compile time.
- `python3 tools/check_exports.py` checks two things. First, the sources are what [tools/gen_exports.py](../tools/gen_exports.py) writes from the pinned headers. Second, every declaration is exported or adapted, and every macro is mapped, kept or an include guard.
- `python3 tools/run_conformance.py` runs FriBidi's own suite on [examples/fribidi](../examples/fribidi), written against the module. That example holds upstream's `fribidi` program ported to the module, and ports of its two conformance runners. The six character-set tests (`fribidi --test --charset CS`, compared with their references byte for byte) pass. So do all 770,241 tests of BidiTest.txt and all 91,707 of BidiCharacterTest.txt. Nothing is excluded: the release ships no fuzzing corpus.
- `python3 tools/test_template.py` scaffolds the default template from an installed copy of the package, then builds and runs it.

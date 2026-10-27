// <mwrap/fribidi/macros.hpp> — FriBidi's macros as macros, for the ones a C++
// entity cannot be: the declaration attributes (FRIBIDI_ENTRY, FRIBIDI_EXTERN,
// FRIBIDI_GNUC_*), the extern "C" brackets (FRIBIDI_BEGIN_DECLS /
// FRIBIDI_END_DECLS), the diagnostic pragmas (FRIBIDI_BEGIN_IGNORE_DEPRECATIONS
// / FRIBIDI_END_IGNORE_DEPRECATIONS) and the X-macro protocol of the *-list.h
// tables (_FRIBIDI_ADD_TYPE, _FRIBIDI_ADD_ALIAS, _FRIBIDI_ADD_CHAR_SET …).
//
// Include it before `import mcpplibs.mwrap.fribidi;`: it is <fribidi.h>, as the
// module sees it (compat.fribidi is a static library, so on Windows its entry
// points are declared without dllimport, FRIBIDI_LIB_STATIC). Every other
// macro is defined too, and while a macro is defined it expands even inside
// `mcpplibs::mwrap::fribidi::macros::NAME`; a function-like one does not when
// its name is followed by `)`: `(macros::FRIBIDI_IS_RTL)(t)` is the function.
#pragma once
#if defined(_WIN32) && !defined(FRIBIDI_LIB_STATIC)
#define FRIBIDI_LIB_STATIC 1
#endif
#include <fribidi.h>

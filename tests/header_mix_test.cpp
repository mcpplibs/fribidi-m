// The raw header and the import in one translation unit (the side header
// first, the canonical order): the module's names are the very same entities
// as the header's; the macros a C++ entity cannot be work beside them — the
// X-macro tables, the deprecation pragmas, the extern "C" brackets and
// FRIBIDI_ENTRY on a declaration of one's own.
#include <mwrap/fribidi/macros.hpp>
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.fribidi;

// A declaration of one's own in FriBidi's style, inside its extern "C" brackets.
FRIBIDI_BEGIN_DECLS
int mwrap_fribidi_test_entry(int value);
FRIBIDI_END_DECLS
int mwrap_fribidi_test_entry(int value) { return value + 1; }

namespace {

// fribidi-bidi-types-list.h, the X-macro table, expanded into a name table.
struct TypeName {
    FriBidiCharType type;
    const char* name;
    char symbol;
};
constexpr TypeName type_names[] = {
#define _FRIBIDI_ADD_TYPE(TYPE, SYMBOL) {FRIBIDI_TYPE_##TYPE, #TYPE, SYMBOL},
#include <fribidi-bidi-types-list.h>
#undef _FRIBIDI_ADD_TYPE
};

// fribidi-joining-types-list.h, likewise.
constexpr std::pair<FriBidiJoiningType, char> joining_symbols[] = {
#define _FRIBIDI_ADD_TYPE(TYPE, SYMBOL) {FRIBIDI_JOINING_TYPE_##TYPE, SYMBOL},
#include <fribidi-joining-types-list.h>
#undef _FRIBIDI_ADD_TYPE
};

// fribidi-char-sets-list.h: every character set, with its identifier in the library's sources.
constexpr std::pair<FriBidiCharSet, const char*> char_sets[] = {
#define _FRIBIDI_ADD_CHAR_SET(CHAR_SET, char_set) {FRIBIDI_CHAR_SET_##CHAR_SET, #char_set},
#include <fribidi-char-sets-list.h>
#undef _FRIBIDI_ADD_CHAR_SET
};

}  // namespace

TEST(HeaderMix, SameEntities) {
    // One declaration each, whichever way it was introduced.
    static_assert(std::is_same_v<decltype(&::fribidi_log2vis), decltype(&fribidi_log2vis)>);
    static_assert(std::is_same_v<::FriBidiCharType, std::uint32_t> && std::is_same_v<::FriBidiLevel, signed char>);
    EXPECT_EQ(&::fribidi_get_bidi_type, &fribidi_get_bidi_type);
    EXPECT_EQ(&::fribidi_version_info, &fribidi_version_info);
    EXPECT_EQ(::fribidi_get_bidi_type(0x05D0), static_cast<FriBidiCharType>(FRIBIDI_TYPE_RTL));
    EXPECT_EQ(mwrap_fribidi_test_entry(41), 42);
}

TEST(HeaderMix, XMacroTables) {
    EXPECT_EQ(std::size(type_names), 23u);  // the 23 bidi types
    for (const auto& t : type_names) {
        EXPECT_STREQ(fribidi_get_bidi_type_name(t.type), t.name);
        EXPECT_TRUE(t.symbol != 0);
    }
    EXPECT_EQ(std::size(joining_symbols), 7u);
    for (auto [type, symbol] : joining_symbols) EXPECT_STRNE(fribidi_get_joining_type_name(type), "?") << symbol;
    EXPECT_EQ(std::size(char_sets), static_cast<std::size_t>(FRIBIDI_CHAR_SETS_NUM));
    for (auto [set, identifier] : char_sets) EXPECT_EQ(fribidi_parse_charset(fribidi_char_set_name(set)), set) << identifier;
}

TEST(HeaderMix, DeprecatedBehindItsPragmas) {
    FRIBIDI_BEGIN_IGNORE_DEPRECATIONS
    EXPECT_EQ(fribidi_get_type(U'a'), fribidi_get_bidi_type(U'a'));
    EXPECT_TRUE(fribidi_mirroring_status());
    FRIBIDI_END_IGNORE_DEPRECATIONS
}

TEST(HeaderMix, MacrosBesideTheMappings) {
    namespace m = mcpplibs::mwrap::fribidi::macros;
    // A function-like macro is not expanded when its name is followed by `)`:
    // here the macro and the function, side by side.
    for (FriBidiCharType t : {FRIBIDI_TYPE_LTR, FRIBIDI_TYPE_RTL, FRIBIDI_TYPE_AN, FRIBIDI_TYPE_PDI}) {
        EXPECT_EQ(FRIBIDI_IS_RTL(t), (m::FRIBIDI_IS_RTL)(t));
        EXPECT_EQ(FRIBIDI_DIR_TO_LEVEL(t), (m::FRIBIDI_DIR_TO_LEVEL)(t));
    }
    // An object-like macro expands even in a qualified name: set it aside to
    // name the constant.
    constexpr auto raw = FRIBIDI_PAR_RTL;
#pragma push_macro("FRIBIDI_PAR_RTL")
#undef FRIBIDI_PAR_RTL
    static_assert(m::FRIBIDI_PAR_RTL == raw);
#pragma pop_macro("FRIBIDI_PAR_RTL")
    // fribidi-common.h declares the debug pair in C++ linkage: the header's
    // ::fribidi_set_debug cannot link; the adaptation calls the C function.
    EXPECT_FALSE(mcpplibs::mwrap::fribidi::fribidi_set_debug(0));
}

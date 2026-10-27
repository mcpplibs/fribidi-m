// The macros module on its own (no header, no root module): the constants
// and functions are constant expressions, as the macros are, so they work in
// constant-initialised tables and static_asserts.
import std;
import mcpplibs.mwrap.fribidi.macros;

namespace macros = mcpplibs::mwrap::fribidi::macros;
using namespace mcpplibs::mwrap::fribidi::macros;

// Every constant has the type its macro expands to in C++.
static_assert(std::is_same_v<decltype(FRIBIDI_TYPE_LTR), const long>);  // 0x00000010L | 0x00000100L
static_assert(std::is_same_v<decltype(FRIBIDI_FLAG_REORDER_NSM), const int>);
static_assert(std::is_same_v<decltype(FRIBIDI_BRACKET_OPEN_MASK), const unsigned int>);  // 0x80000000
static_assert(std::is_same_v<decltype(FRIBIDI_VERSION), const char (&)[7]>);

static_assert(FRIBIDI_TYPE_RTL == (FRIBIDI_MASK_STRONG | FRIBIDI_MASK_LETTER | FRIBIDI_MASK_RTL));
static_assert(FRIBIDI_PAR_LTR == FRIBIDI_TYPE_LTR && FRIBIDI_PAR_ON == FRIBIDI_TYPE_ON);
static_assert(FRIBIDI_FLAGS_ARABIC == (FRIBIDI_FLAG_SHAPE_ARAB_PRES | FRIBIDI_FLAG_SHAPE_ARAB_LIGA));
static_assert(FRIBIDI_JOINING_TYPE_D_VAL == (FRIBIDI_MASK_JOINS_RIGHT | FRIBIDI_MASK_JOINS_LEFT | FRIBIDI_MASK_ARAB_SHAPES));
static_assert(FRIBIDI_CHAR_LRM == 0x200E && FRIBIDI_CHAR_PDI == 0x2069 && UNI_FARSI_ZERO == FRIBIDI_CHAR_PERSIAN_ZERO);
static_assert(FRIBIDI_BIDI_MAX_RESOLVED_LEVELS == 127 && FRIBIDI_BIDI_MAX_NESTED_BRACKET_PAIRS == 63);
static_assert(std::string_view(FRIBIDI_VERSION) == "1.0.16" && std::string_view(FRIBIDI_UNICODE_VERSION) == "16.0.0");
static_assert(std::string_view(FRIBIDI_NAME) == "GNU FriBidi" && FRIBIDI_SIZEOF_INT == sizeof(int));

// The functions, at compile time.
static_assert(FRIBIDI_IS_RTL(FRIBIDI_TYPE_AL) && !FRIBIDI_IS_RTL(FRIBIDI_TYPE_EN));
static_assert(FRIBIDI_IS_LETTER(FRIBIDI_TYPE_LTR) && FRIBIDI_IS_NUMBER(FRIBIDI_TYPE_AN));
static_assert(FRIBIDI_LEVEL_IS_RTL(3) && !FRIBIDI_LEVEL_IS_RTL(4));
static_assert(FRIBIDI_LEVEL_TO_DIR(1) == FRIBIDI_TYPE_RTL && FRIBIDI_LEVEL_TO_DIR(2) == FRIBIDI_TYPE_LTR);
static_assert(FRIBIDI_DIR_TO_LEVEL(FRIBIDI_TYPE_RLE) == 1);
static_assert(std::is_same_v<decltype(FRIBIDI_DIR_TO_LEVEL(FRIBIDI_TYPE_RLE)), signed char>);  // (FriBidiLevel)
static_assert(FRIBIDI_IS_JOINING_TYPE_R(FRIBIDI_JOINING_TYPE_R_VAL) && FRIBIDI_IS_JOIN_BASE_SHAPES(FRIBIDI_JOINING_TYPE_L_VAL));
static_assert(FRIBIDI_JOINS_PRECEDING_MASK(1) == FRIBIDI_MASK_JOINS_RIGHT);
static_assert(FRIBIDI_IS_BRACKET_OPEN(0x80000028u) && FRIBIDI_BRACKET_ID(0x80000028u) == 0x28u);
// Upstream's precedence, kept: `==` binds tighter than `&`, so
// FRIBIDI_IS_LTR_LETTER is `p & 0` and FRIBIDI_IS_RTL_LETTER is `p & 1`.
static_assert(FRIBIDI_IS_LTR_LETTER(FRIBIDI_TYPE_LTR) == 0 && FRIBIDI_IS_RTL_LETTER(FRIBIDI_TYPE_AL) == 1);

constexpr unsigned flags_after() {
    unsigned flags = 0;
    FRIBIDI_SET_BITS(flags, FRIBIDI_FLAG_SHAPE_MIRRORING);
    FRIBIDI_ADJUST_BITS(flags, FRIBIDI_FLAG_REORDER_NSM, true);
    FRIBIDI_UNSET_BITS(flags, FRIBIDI_FLAG_SHAPE_MIRRORING);
    return FRIBIDI_ADJUST_AND_TEST_BITS(flags, FRIBIDI_FLAG_REMOVE_BIDI, false) ? 0u : flags;
}
static_assert(flags_after() == FRIBIDI_FLAG_REORDER_NSM);

// A constant-initialised table, as the macros allow.
struct Class {
    long type;
    bool rtl, strong;
};
constexpr Class classes[] = {
    {FRIBIDI_TYPE_LTR, FRIBIDI_IS_RTL(FRIBIDI_TYPE_LTR) != 0, FRIBIDI_IS_STRONG(FRIBIDI_TYPE_LTR) != 0},
    {FRIBIDI_TYPE_RTL, FRIBIDI_IS_RTL(FRIBIDI_TYPE_RTL) != 0, FRIBIDI_IS_STRONG(FRIBIDI_TYPE_RTL) != 0},
    {FRIBIDI_TYPE_WS, FRIBIDI_IS_RTL(FRIBIDI_TYPE_WS) != 0, FRIBIDI_IS_STRONG(FRIBIDI_TYPE_WS) != 0},
};
static_assert(!classes[0].rtl && classes[1].rtl && classes[1].strong && !classes[2].strong);

int main() {
    return macros::FRIBIDI_MAJOR_VERSION == 1 && macros::FRIBIDI_UNICODE_CHARS == 0x110000 ? 0 : 1;
}

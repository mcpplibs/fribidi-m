// The FriBidi API through the module, spelled as <fribidi.h> documents it:
// character properties (bidi types, joining types, mirrors, brackets) and
// their names, paragraph direction and embedding levels (with and without
// bracket pairs, UAX #9 N0), line reordering, log2vis with its maps,
// mirroring and Arabic joining and shaping (presentation forms and the
// lam-alef ligature), the character sets, the deprecated API and the
// versions. Only the module is imported; no FriBidi header is included.
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.fribidi;

using namespace mcpplibs::mwrap::fribidi::macros;  // FRIBIDI_TYPE_LTR, FRIBIDI_PAR_ON, FRIBIDI_IS_RTL …

namespace {

std::vector<FriBidiChar> text(std::u32string_view s) { return {s.begin(), s.end()}; }

std::u32string chars(const FriBidiChar* p, std::size_t n) {
    std::u32string out;
    for (std::size_t i = 0; i < n; ++i) out.push_back(static_cast<char32_t>(p[i]));
    return out;
}

FriBidiStrIndex length(const std::vector<FriBidiChar>& s) { return static_cast<FriBidiStrIndex>(s.size()); }

// Levels of a paragraph, as a renderer computes them: types, bracket types, levels.
struct Paragraph {
    std::vector<FriBidiChar> str;
    std::vector<FriBidiCharType> types;
    std::vector<FriBidiBracketType> brackets;
    std::vector<FriBidiLevel> levels;
    FriBidiParType base;
    FriBidiLevel max_level_plus_one;

    Paragraph(std::u32string_view s, FriBidiParType direction, bool with_brackets = true)
        : str(text(s)), types(str.size()), brackets(str.size()), levels(str.size()), base(direction) {
        fribidi_get_bidi_types(str.data(), length(str), types.data());
        fribidi_get_bracket_types(str.data(), length(str), types.data(), brackets.data());
        max_level_plus_one = fribidi_get_par_embedding_levels_ex(types.data(), with_brackets ? brackets.data() : nullptr,
                                                                 length(str), &base, levels.data());
    }
    std::vector<int> level_list() const { return {levels.begin(), levels.end()}; }
};

}  // namespace

TEST(Versions, LibraryAndUnicode) {
    EXPECT_NE(std::string_view(fribidi_version_info).find("1.0.16"), std::string_view::npos);
    EXPECT_EQ(std::string_view(fribidi_unicode_version), "16.0.0");
    EXPECT_EQ(std::string_view(FRIBIDI_VERSION), "1.0.16");
    EXPECT_EQ(std::string_view(FRIBIDI_UNICODE_VERSION), fribidi_unicode_version);
    EXPECT_EQ(FRIBIDI_MAJOR_VERSION * 10000 + FRIBIDI_MINOR_VERSION * 100 + FRIBIDI_MICRO_VERSION, 10016);
    EXPECT_EQ(FRIBIDI_UNICODE_MAJOR_VERSION, 16);
    EXPECT_EQ(FRIBIDI_INTERFACE_VERSION, 4);
}

TEST(BidiTypes, OfEveryClass) {
    const std::pair<char32_t, FriBidiCharType> table[] = {
        {U'a', FRIBIDI_TYPE_LTR},   {U'א', FRIBIDI_TYPE_RTL}, {U'ا', FRIBIDI_TYPE_AL}, {U'1', FRIBIDI_TYPE_EN},
        {U'١', FRIBIDI_TYPE_AN}, {U'+', FRIBIDI_TYPE_ES},     {U'$', FRIBIDI_TYPE_ET},      {U',', FRIBIDI_TYPE_CS},
        {U'\u0300', FRIBIDI_TYPE_NSM}, {U'\u200B', FRIBIDI_TYPE_BN}, {U'\n', FRIBIDI_TYPE_BS},     {U'\t', FRIBIDI_TYPE_SS},
        {U' ', FRIBIDI_TYPE_WS},    {U'!', FRIBIDI_TYPE_ON},       {U'\u202A', FRIBIDI_TYPE_LRE}, {U'\u202B', FRIBIDI_TYPE_RLE},
        {U'\u202D', FRIBIDI_TYPE_LRO}, {U'\u202E', FRIBIDI_TYPE_RLO}, {U'\u202C', FRIBIDI_TYPE_PDF}, {U'\u2066', FRIBIDI_TYPE_LRI},
        {U'\u2067', FRIBIDI_TYPE_RLI}, {U'\u2068', FRIBIDI_TYPE_FSI}, {U'\u2069', FRIBIDI_TYPE_PDI},
    };
    std::u32string all;
    for (auto [c, type] : table) {
        EXPECT_EQ(fribidi_get_bidi_type(c), type) << std::hex << static_cast<std::uint32_t>(c);
        all.push_back(c);
    }
    auto str = text(all);
    std::vector<FriBidiCharType> types(str.size());
    fribidi_get_bidi_types(str.data(), length(str), types.data());
    for (std::size_t i = 0; i < str.size(); ++i) EXPECT_EQ(types[i], table[i].second);

    EXPECT_STREQ(fribidi_get_bidi_type_name(FRIBIDI_TYPE_LTR), "LTR");
    EXPECT_STREQ(fribidi_get_bidi_type_name(FRIBIDI_TYPE_AL), "AL");
    EXPECT_STREQ(fribidi_get_bidi_type_name(FRIBIDI_TYPE_PDI), "PDI");
    EXPECT_STREQ(fribidi_get_bidi_type_name(FRIBIDI_PAR_WRTL), "WRTL");
    EXPECT_STREQ(fribidi_get_bidi_type_name(0x7fff), "?");

    // The classification macros, as functions.
    EXPECT_TRUE(FRIBIDI_IS_RTL(FRIBIDI_TYPE_AL) && FRIBIDI_IS_ARABIC(FRIBIDI_TYPE_AN));
    EXPECT_TRUE(FRIBIDI_IS_STRONG(FRIBIDI_TYPE_LTR) && FRIBIDI_IS_WEAK(FRIBIDI_TYPE_EN) && FRIBIDI_IS_NEUTRAL(FRIBIDI_TYPE_WS));
    EXPECT_TRUE(FRIBIDI_IS_ISOLATE(FRIBIDI_TYPE_FSI) && FRIBIDI_IS_EXPLICIT(FRIBIDI_TYPE_RLO) && FRIBIDI_IS_OVERRIDE(FRIBIDI_TYPE_RLO));
    EXPECT_TRUE(FRIBIDI_IS_EXPLICIT_OR_BN(fribidi_get_bidi_type(0x200B)));
    EXPECT_EQ(FRIBIDI_DIR_TO_LEVEL(FRIBIDI_TYPE_RTL), 1);
    EXPECT_EQ(FRIBIDI_LEVEL_TO_DIR(3), FRIBIDI_TYPE_RTL);
    EXPECT_EQ(FRIBIDI_CHANGE_NUMBER_TO_RTL(FRIBIDI_TYPE_EN), FRIBIDI_TYPE_RTL);
    EXPECT_EQ(FRIBIDI_EXPLICIT_TO_OVERRIDE_DIR(FRIBIDI_TYPE_RLO), FRIBIDI_TYPE_RTL);
    EXPECT_EQ(FRIBIDI_WEAK_PARAGRAPH(FRIBIDI_PAR_RTL), FRIBIDI_PAR_WRTL);
}

TEST(Paragraph, DirectionOfTheFirstStrongCharacter) {
    auto direction = [](std::u32string_view s) {
        auto str = text(s);
        std::vector<FriBidiCharType> types(str.size());
        fribidi_get_bidi_types(str.data(), length(str), types.data());
        return fribidi_get_par_direction(types.data(), length(str));
    };
    EXPECT_EQ(direction(U"123 abc א"), FRIBIDI_PAR_LTR);
    EXPECT_EQ(direction(U"123 א abc"), FRIBIDI_PAR_RTL);
    EXPECT_EQ(direction(U"ا"), FRIBIDI_PAR_RTL);
    EXPECT_EQ(direction(U"123 !"), FRIBIDI_PAR_ON);
    EXPECT_EQ(direction(U"\u2067א\u2069 a"), FRIBIDI_PAR_LTR);  // an isolate is skipped
}

TEST(Paragraph, EmbeddingLevels) {
    Paragraph ltr(U"abc אבג 123", FRIBIDI_PAR_ON);
    EXPECT_EQ(ltr.base, FRIBIDI_PAR_LTR);
    // The numbers follow R (W7 does not make them L), so the space between is R
    // (N1) and they take level 2 (I1).
    EXPECT_EQ(ltr.level_list(), (std::vector<int>{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2}));
    EXPECT_EQ(ltr.max_level_plus_one, 3);

    Paragraph rtl(U"אב abc 12", FRIBIDI_PAR_ON);
    EXPECT_EQ(rtl.base, FRIBIDI_PAR_RTL);
    // The numbers follow L (W7: EN becomes L), so the space between is L too.
    EXPECT_EQ(rtl.level_list(), (std::vector<int>{1, 1, 1, 2, 2, 2, 2, 2, 2}));
    EXPECT_EQ(rtl.max_level_plus_one, 3);

    Paragraph weak(U"123", FRIBIDI_PAR_WRTL);  // no strong character: the weak direction stays
    EXPECT_EQ(weak.base, FRIBIDI_PAR_WRTL);
    EXPECT_EQ(FRIBIDI_DIR_TO_LEVEL(weak.base), 1);
    EXPECT_EQ(weak.level_list(), (std::vector<int>{2, 2, 2}));

    Paragraph forced(U"abc", FRIBIDI_PAR_RTL);
    EXPECT_EQ(forced.level_list(), (std::vector<int>{2, 2, 2}));
    EXPECT_TRUE(FRIBIDI_LEVEL_IS_RTL(forced.levels[0]) == 0);

    Paragraph embedded(U"a\u202Bb\u202Cc", FRIBIDI_PAR_LTR);  // RLE … PDF raises b to level 1 (and L in it to 2)
    EXPECT_EQ(embedded.levels[2], 2);
}

TEST(Paragraph, BracketPairsResolveTogether) {
    // UAX #9 N0's example "AB(CD[&ef]!)gh" in an RTL paragraph, with Hebrew for
    // the capitals: the pair [ ] around L text takes the R of what precedes it.
    constexpr std::u32string_view s = U"אב(גד[&ef]!)gh";
    Paragraph pairs(s, FRIBIDI_PAR_RTL);
    EXPECT_EQ(pairs.level_list(), (std::vector<int>{1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 2, 2}));
    Paragraph unpaired(s, FRIBIDI_PAR_RTL, false);
    EXPECT_EQ(unpaired.level_list(), (std::vector<int>{1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2}));

    const FriBidiBracketType open = fribidi_get_bracket(U'('), close = fribidi_get_bracket(U')');
    EXPECT_TRUE(FRIBIDI_IS_BRACKET_OPEN(open));
    EXPECT_FALSE(FRIBIDI_IS_BRACKET_OPEN(close));
    EXPECT_EQ(FRIBIDI_BRACKET_ID(open), FRIBIDI_BRACKET_ID(close));
    EXPECT_NE(FRIBIDI_BRACKET_ID(open), FRIBIDI_BRACKET_ID(fribidi_get_bracket(U'[')));
    EXPECT_EQ(fribidi_get_bracket(U'a'), static_cast<FriBidiBracketType>(FRIBIDI_NO_BRACKET));
    EXPECT_EQ(pairs.brackets[0], static_cast<FriBidiBracketType>(FRIBIDI_NO_BRACKET));
    EXPECT_EQ(pairs.brackets[2], open);
}

TEST(Reorder, LineToVisualOrder) {
    Paragraph p(U"abc אבג def", FRIBIDI_PAR_ON);
    std::vector<FriBidiStrIndex> map(p.str.size());
    std::iota(map.begin(), map.end(), 0);
    std::vector<FriBidiChar> visual = p.str;
    const FriBidiLevel result = fribidi_reorder_line(FRIBIDI_FLAGS_DEFAULT, p.types.data(), length(p.str), 0, p.base,
                                                     p.levels.data(), visual.data(), map.data());
    EXPECT_EQ(result, 2);
    EXPECT_EQ(map, (std::vector<FriBidiStrIndex>{0, 1, 2, 3, 6, 5, 4, 7, 8, 9, 10}));
    EXPECT_EQ(chars(visual.data(), visual.size()), U"abc גבא def");
}

TEST(Log2Vis, VisualStringMapsAndMirroring) {
    auto logical = text(U"א(ב)");
    FriBidiParType base = FRIBIDI_PAR_ON;
    std::vector<FriBidiChar> visual(logical.size() + 1);
    std::vector<FriBidiStrIndex> ltov(logical.size() + 1), vtol(logical.size() + 1);
    std::vector<FriBidiLevel> levels(logical.size() + 1);
    const FriBidiLevel max = fribidi_log2vis(logical.data(), length(logical), &base, visual.data(), ltov.data(),
                                             vtol.data(), levels.data());
    EXPECT_EQ(max, 2);
    EXPECT_EQ(base, FRIBIDI_PAR_RTL);
    EXPECT_EQ(chars(visual.data(), logical.size()), U"(ב)א");  // reversed, its brackets mirrored
    EXPECT_EQ(std::vector<FriBidiStrIndex>(ltov.begin(), ltov.begin() + 4), (std::vector<FriBidiStrIndex>{3, 2, 1, 0}));
    EXPECT_EQ(std::vector<FriBidiStrIndex>(vtol.begin(), vtol.begin() + 4), (std::vector<FriBidiStrIndex>{3, 2, 1, 0}));

    FriBidiChar mirrored = 0;
    EXPECT_TRUE(fribidi_get_mirror_char(U'(', &mirrored));
    EXPECT_EQ(mirrored, static_cast<FriBidiChar>(U')'));
    EXPECT_TRUE(fribidi_get_mirror_char(U'≮', &mirrored));  // NOT LESS-THAN
    EXPECT_EQ(mirrored, 0x226Fu);
    EXPECT_FALSE(fribidi_get_mirror_char(U'a', &mirrored));
    EXPECT_EQ(mirrored, static_cast<FriBidiChar>(U'a'));
}

TEST(Log2Vis, RemovingBidiMarks) {
    auto s = text(U"a\u200Eb\u202Bc\u202C");
    std::vector<FriBidiStrIndex> to(s.size()), from(s.size());
    std::iota(to.begin(), to.end(), 0);
    std::iota(from.begin(), from.end(), 0);
    std::vector<FriBidiLevel> levels(s.size(), 0);
    const FriBidiStrIndex n = fribidi_remove_bidi_marks(s.data(), length(s), to.data(), from.data(), levels.data());
    EXPECT_EQ(n, 3);
    EXPECT_EQ(chars(s.data(), static_cast<std::size_t>(n)), U"abc");
    EXPECT_EQ(std::vector<FriBidiStrIndex>(from.begin(), from.begin() + 3), (std::vector<FriBidiStrIndex>{0, 2, 4}));
}

TEST(Shaping, MirroringOfRtlLevels) {
    auto s = text(U"(a)[b]");
    const std::vector<FriBidiLevel> levels{1, 1, 1, 0, 0, 0};
    fribidi_shape_mirroring(levels.data(), length(s), s.data());
    EXPECT_EQ(chars(s.data(), s.size()), U")a([b]");
}

TEST(Shaping, ArabicJoiningTypes) {
    const std::pair<char32_t, FriBidiJoiningType> table[] = {
        {U'ب', FRIBIDI_JOINING_TYPE_D}, {U'ا', FRIBIDI_JOINING_TYPE_R}, {U'a', FRIBIDI_JOINING_TYPE_U},
        {U'\u064E', FRIBIDI_JOINING_TYPE_T}, {U'ـ', FRIBIDI_JOINING_TYPE_C}, {U'\u200D', FRIBIDI_JOINING_TYPE_C},
        {U'\u0001', FRIBIDI_JOINING_TYPE_G},  // BN, not a format character (those are T)
        {U'\u202B', FRIBIDI_JOINING_TYPE_T},
    };
    std::u32string all;
    for (auto [c, type] : table) {
        EXPECT_EQ(fribidi_get_joining_type(c), type);
        all.push_back(c);
    }
    auto s = text(all);
    std::vector<FriBidiJoiningType> types(s.size());
    fribidi_get_joining_types(s.data(), length(s), types.data());
    for (std::size_t i = 0; i < s.size(); ++i) EXPECT_EQ(types[i], table[i].second);
    EXPECT_STREQ(fribidi_get_joining_type_name(FRIBIDI_JOINING_TYPE_D), "D");
    EXPECT_STREQ(fribidi_get_joining_type_name(FRIBIDI_JOINING_TYPE_T), "T");
    EXPECT_TRUE(FRIBIDI_IS_JOINING_TYPE_D(FRIBIDI_JOINING_TYPE_D) && FRIBIDI_IS_JOINING_TYPE_R(FRIBIDI_JOINING_TYPE_R));
    EXPECT_TRUE(FRIBIDI_JOINS_RIGHT(FRIBIDI_JOINING_TYPE_R) && !FRIBIDI_JOINS_LEFT(FRIBIDI_JOINING_TYPE_R));
    EXPECT_TRUE(FRIBIDI_IS_JOIN_SKIPPED(FRIBIDI_JOINING_TYPE_T) && FRIBIDI_ARAB_SHAPES(FRIBIDI_JOINING_TYPE_D));
    EXPECT_EQ(FRIBIDI_JOINING_TYPE_D_VAL, FRIBIDI_JOINING_TYPE_D);
}

namespace {
// fribidi's Arabic pipeline: types, levels, joining types, joining, shaping.
std::u32string shaped(std::u32string_view s, FriBidiFlags flags, bool through_fribidi_shape = false) {
    Paragraph p(s, FRIBIDI_PAR_ON);
    std::vector<FriBidiArabicProp> props(p.str.size());
    fribidi_get_joining_types(p.str.data(), length(p.str), props.data());
    fribidi_join_arabic(p.types.data(), length(p.str), p.levels.data(), props.data());
    if (through_fribidi_shape)
        fribidi_shape(flags, p.levels.data(), length(p.str), props.data(), p.str.data());
    else
        fribidi_shape_arabic(flags, p.levels.data(), length(p.str), props.data(), p.str.data());
    return chars(p.str.data(), p.str.size());
}
}  // namespace

TEST(Shaping, ArabicPresentationForms) {
    // BEH BEH: initial then final; BEH ALEF BEH: initial, final (ALEF is
    // right-joining, so the last BEH stands alone).
    EXPECT_EQ(shaped(U"بب", FRIBIDI_FLAG_SHAPE_ARAB_PRES), U"\uFE91\uFE90");
    EXPECT_EQ(shaped(U"باب", FRIBIDI_FLAG_SHAPE_ARAB_PRES), U"\uFE91\uFE8E\uFE8F");
    EXPECT_EQ(shaped(U"ببب", FRIBIDI_FLAG_SHAPE_ARAB_PRES), U"\uFE91\uFE92\uFE90");
    // LAM ALEF: the ligature in ALEF's slot, LAM's left for removal (FRIBIDI_CHAR_FILL).
    EXPECT_EQ(shaped(U"لا", FRIBIDI_FLAGS_ARABIC), (std::u32string{static_cast<char32_t>(FRIBIDI_CHAR_FILL), U'\uFEFB'}));
    EXPECT_EQ(shaped(U"لا", FRIBIDI_FLAG_SHAPE_ARAB_PRES), U"\uFEDF\uFE8E");
    // fribidi_shape: mirroring and Arabic shaping together.
    EXPECT_EQ(shaped(U"(بب)", FRIBIDI_FLAGS_DEFAULT | FRIBIDI_FLAGS_ARABIC, true), U")\uFE91\uFE90(");
}

TEST(CharSets, ParseNameAndConvert) {
    EXPECT_EQ(fribidi_parse_charset("UTF-8"), FRIBIDI_CHAR_SET_UTF8);
    EXPECT_EQ(fribidi_parse_charset("utf-8"), FRIBIDI_CHAR_SET_UTF8);
    EXPECT_EQ(fribidi_parse_charset("CapRTL"), FRIBIDI_CHAR_SET_CAP_RTL);
    EXPECT_EQ(fribidi_parse_charset("ISO8859-6"), FRIBIDI_CHAR_SET_ISO8859_6);
    EXPECT_EQ(fribidi_parse_charset("ISO8859-8"), FRIBIDI_CHAR_SET_ISO8859_8);
    EXPECT_EQ(fribidi_parse_charset("CP1255"), FRIBIDI_CHAR_SET_CP1255);
    EXPECT_EQ(fribidi_parse_charset("CP1256"), FRIBIDI_CHAR_SET_CP1256);
    EXPECT_EQ(fribidi_parse_charset("EBCDIC"), _FRIBIDI_CHAR_SET_NOT_FOUND);
    EXPECT_EQ(FRIBIDI_CHAR_SET_NOT_FOUND, _FRIBIDI_CHAR_SET_NOT_FOUND);
    EXPECT_EQ(FRIBIDI_CHAR_SETS_NUM, 6);
    EXPECT_EQ(_FRIBIDI_CHAR_SETS_NUM_PLUS_ONE, 7);
    EXPECT_STREQ(fribidi_char_set_name(FRIBIDI_CHAR_SET_UTF8), "UTF-8");
    EXPECT_STREQ(fribidi_char_set_title(FRIBIDI_CHAR_SET_CP1255), "CP1255 (MS Hebrew/Yiddish)");
    EXPECT_NE(fribidi_char_set_desc(FRIBIDI_CHAR_SET_CAP_RTL), nullptr);
    EXPECT_EQ(fribidi_char_set_desc(FRIBIDI_CHAR_SET_UTF8), nullptr);

    auto round_trip = [](FriBidiCharSet set, std::string_view bytes, std::u32string_view expected) {
        std::vector<FriBidiChar> us(bytes.size() + 1);
        const FriBidiStrIndex n = fribidi_charset_to_unicode(set, bytes.data(), static_cast<FriBidiStrIndex>(bytes.size()), us.data());
        if (set == FRIBIDI_CHAR_SET_CAP_RTL) {  // a character of the bidi type the letter stands for
            for (std::size_t i = 0; i < expected.size(); ++i)
                EXPECT_EQ(fribidi_get_bidi_type(us[i]), fribidi_get_bidi_type(expected[i])) << i;
        } else {
            EXPECT_EQ(chars(us.data(), static_cast<std::size_t>(n)), expected) << fribidi_char_set_name(set);
        }
        std::string back(bytes.size() * 4 + 1, '\0');
        const FriBidiStrIndex m = fribidi_unicode_to_charset(set, us.data(), n, back.data());
        EXPECT_EQ(std::string_view(back.data(), static_cast<std::size_t>(m)), bytes) << fribidi_char_set_name(set);
    };
    round_trip(FRIBIDI_CHAR_SET_UTF8, "a\xD7\x90\xF0\x9F\x98\x80", U"aא\U0001F600");
    round_trip(FRIBIDI_CHAR_SET_ISO8859_8, "a\xE0", U"aא");
    round_trip(FRIBIDI_CHAR_SET_CP1255, "a\xE0", U"aא");
    round_trip(FRIBIDI_CHAR_SET_ISO8859_6, "a\xC7", U"aا");
    round_trip(FRIBIDI_CHAR_SET_CP1256, "a\xC7", U"aا");
    round_trip(FRIBIDI_CHAR_SET_CAP_RTL, "aBH1", U"aاב1");  // CapRTL: B is AL, H is R
}

TEST(Deprecated, StillProvided) {
    EXPECT_TRUE(fribidi_mirroring_status());
    EXPECT_FALSE(fribidi_set_mirroring(false));
    EXPECT_FALSE(fribidi_mirroring_status());
    EXPECT_TRUE(fribidi_set_mirroring(true));
    EXPECT_TRUE(fribidi_reorder_nsm_status());  // FRIBIDI_FLAGS_DEFAULT has it
    EXPECT_FALSE(fribidi_set_reorder_nsm(false));
    EXPECT_FALSE(fribidi_reorder_nsm_status());
    EXPECT_TRUE(fribidi_set_reorder_nsm(true));
    EXPECT_EQ(fribidi_get_type(U'א'), FRIBIDI_TYPE_RTL);
    EXPECT_EQ(fribidi_get_type_internal(U'1'), FRIBIDI_TYPE_EN);
    EXPECT_EQ(UNI_LRM, FRIBIDI_CHAR_LRM);
    EXPECT_EQ(FRIBIDI_TYPE_R, FRIBIDI_PAR_RTL);

    Paragraph p(U"abc אב", FRIBIDI_PAR_ON, false);
    FriBidiParType base = FRIBIDI_PAR_ON;
    std::vector<FriBidiLevel> levels(p.str.size());
    EXPECT_EQ(fribidi_get_par_embedding_levels(p.types.data(), length(p.str), &base, levels.data()), 2);
    EXPECT_EQ(levels, p.levels);
    base = FRIBIDI_PAR_ON;
    EXPECT_EQ(fribidi_log2vis_get_embedding_levels(p.types.data(), length(p.str), &base, levels.data()), 2);
    EXPECT_EQ(levels, p.levels);
}

TEST(Debug, AdaptedInTheWrapperNamespace) {
    // fribidi-common.h declares these two in C++ linkage (outside its extern
    // "C" block): the header's declarations cannot link. The adaptations call
    // the library's C functions.
    namespace fb = mcpplibs::mwrap::fribidi;
    EXPECT_FALSE(fb::fribidi_debug_status());
    EXPECT_FALSE(fb::fribidi_set_debug(true));  // the library is built without DEBUG
    static_assert(std::is_same_v<decltype(&fb::fribidi_set_debug), int (*)(int)>);
}

TEST(Macros, ConstantsAndBits) {
    static_assert(FRIBIDI_TYPE_LTR == (FRIBIDI_MASK_STRONG | FRIBIDI_MASK_LETTER));
    static_assert(FRIBIDI_PAR_RTL == FRIBIDI_TYPE_RTL_VAL && FRIBIDI_TYPE_WR == FRIBIDI_PAR_WRTL);
    static_assert(FRIBIDI_FLAGS_DEFAULT == (FRIBIDI_FLAG_SHAPE_MIRRORING | FRIBIDI_FLAG_REORDER_NSM | FRIBIDI_FLAG_REMOVE_SPECIALS));
    static_assert(FRIBIDI_UNICODE_CHARS == 0x110000 && FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL == 125);
    static_assert(FRIBIDI_CHAR_FILL == FRIBIDI_CHAR_ZWNBSP && FRIBIDI_CHAR_RLI == 0x2067);
    FriBidiFlags flags = 0;
    EXPECT_EQ(&FRIBIDI_SET_BITS(flags, FRIBIDI_FLAG_REORDER_NSM), &flags);
    EXPECT_EQ(flags, static_cast<FriBidiFlags>(FRIBIDI_FLAG_REORDER_NSM));
    EXPECT_EQ(FRIBIDI_ADJUST_AND_TEST_BITS(flags, FRIBIDI_FLAG_SHAPE_MIRRORING, true), 1);
    EXPECT_EQ(FRIBIDI_TEST_BITS(flags, FRIBIDI_FLAG_SHAPE_MIRRORING), 1);
    FRIBIDI_UNSET_BITS(flags, FRIBIDI_FLAG_REORDER_NSM);
    EXPECT_EQ(flags, static_cast<FriBidiFlags>(FRIBIDI_FLAG_SHAPE_MIRRORING));
    EXPECT_EQ(FRIBIDI_INCLUDE_BITS(flags, 4) & 4, 4u);
    EXPECT_EQ(FRIBIDI_EXCLUDE_BITS(flags, FRIBIDI_FLAG_SHAPE_MIRRORING), 0u);
}

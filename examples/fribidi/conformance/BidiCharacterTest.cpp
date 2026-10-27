// A port of GNU FriBidi's test/unicode-conformance/BidiCharacterTest.c, and
// so under its license:
//
// Copyright (C) 2015, 2017 Dov Grobgeld
// Port to C++ modules: Copyright (c) 2026 mcpp-community contributors
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version. It is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
// General Public License (upstream/fribidi/COPYING) for more details.

// BidiCharacterTest — FriBidi's runner of the Unicode conformance file
// BidiCharacterTest.txt (test/unicode-conformance/BidiCharacterTest.c of
// the 1.0.16 release), written against `import mcpplibs.mwrap.fribidi;`.
//
// Each line: code points; paragraph direction (0 LTR, 1 RTL, 2 auto); the
// resolved paragraph level; the expected levels (x: removed by rule X9); the
// expected visual order. As upstream's runner: the bidi type of each
// character, its bracket type when it is ON (brackets are always neutral),
// levels from fribidi_get_par_embedding_levels_ex, the order from
// fribidi_reorder_line with no flags, X9-removed characters dropped from it.
//
//   BidiCharacterTest [--debug] BidiCharacterTest.txt
//
// Prints each failure as upstream does, then a summary; exits 1 on any failure.
import std;
import mcpplibs.mwrap.fribidi;

using namespace mcpplibs::mwrap::fribidi::macros;

namespace {

[[noreturn]] void die(std::string_view message) {
    std::cerr << message;
    std::exit(255);
}

std::vector<std::string_view> fields(std::string_view line) {
    std::vector<std::string_view> out;
    for (auto part : std::views::split(line, ';')) out.emplace_back(part.begin(), part.end());
    return out;
}

std::vector<int> numbers(std::string_view s, bool x_allowed, int base = 10) {
    std::vector<int> out;
    std::size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        if (i == s.size()) break;
        if (x_allowed && s[i] == 'x') {
            out.push_back(-1);
            ++i;
            continue;
        }
        int value = 0;
        auto [end, ec] = std::from_chars(s.data() + i, s.data() + s.size(), value, base);
        if (ec != std::errc{}) die("Oops! I shouldn't be here!\n");
        out.push_back(value);
        i = static_cast<std::size_t>(end - s.data());
    }
    return out;
}

std::string join(const auto& values, bool x_for_minus_one = false) {
    std::string out;
    for (auto v : values) out += (x_for_minus_one && v == -1) ? std::string(" x") : std::format(" {}", static_cast<int>(v));
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("usage: {} [--debug] test-file-name\n", argv[0]);
        return 1;
    }
    int next_arg = 1;
    bool debug = false;
    while (next_arg < argc && argv[next_arg][0] == '-') {
        const std::string_view arg = argv[next_arg++];
        if (arg == "--debug") {
            debug = true;
            continue;
        }
        die(std::format("Unknown option {}!\n", arg));
    }
    if (next_arg >= argc) die(std::format("usage: {} [--debug] test-file-name\n", argv[0]));
    const char* filename = argv[next_arg];
    std::ifstream channel(filename, std::ios::binary);
    if (!channel) die(std::format("Failed opening {}\n", filename));

    int numerrs = 0, numtests = 0, line_no = 0;
    std::string line;
    while (std::getline(channel, line)) {
        line_no++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto f = fields(line);
        if (f.size() < 5) die(std::format("Oops! Didn't find expected ; at line {}\n", line_no));

        std::vector<FriBidiChar> code_points;
        for (int c : numbers(f[0], false, 16)) code_points.push_back(static_cast<FriBidiChar>(c));
        const int paragraph_dir = std::atoi(std::string(f[1]).c_str());
        const std::vector<int> expected_levels = numbers(f[3], true);
        const std::vector<int> expected_ltor = numbers(f[4], false);
        if (expected_levels.size() != code_points.size())
            die(std::format("Oops! Different lengths for levels and codepoints at line {}!\n", line_no));

        const auto n = code_points.size();
        const auto len = static_cast<FriBidiStrIndex>(n);
        std::vector<FriBidiCharType> types(n);
        std::vector<FriBidiBracketType> bracket_types(n);
        std::vector<FriBidiLevel> levels(n);
        std::vector<FriBidiStrIndex> ltor(n);
        for (std::size_t i = 0; i < n; i++) {
            types[i] = fribidi_get_bidi_type(code_points[i]);
            // Note the optimization that a bracket is always of type neutral.
            bracket_types[i] = types[i] == FRIBIDI_TYPE_ON ? fribidi_get_bracket(code_points[i]) : FRIBIDI_NO_BRACKET;
        }
        numtests++;
        FriBidiParType base_dir = paragraph_dir == 0 ? FRIBIDI_PAR_LTR : paragraph_dir == 1 ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_ON;
        if (fribidi_get_par_embedding_levels_ex(types.data(), bracket_types.data(), len, &base_dir, levels.data())) {
        }
        std::iota(ltor.begin(), ltor.end(), 0);
        if (fribidi_reorder_line(0, types.data(), len, 0, base_dir, levels.data(), nullptr, ltor.data())) {
        }
        std::vector<int> order;
        for (auto index : ltor)
            if (!FRIBIDI_IS_EXPLICIT_OR_BN(types[static_cast<std::size_t>(index)])) order.push_back(index);

        bool matches = true;
        for (std::size_t i = 0; i < n; i++)
            if (levels[i] != expected_levels[i] && expected_levels[i] != -1) {
                matches = false;
                break;
            }
        if (order != expected_ltor) matches = false;

        if (!matches) {
            numerrs++;
            std::cerr << std::format("failure on line {}\ninput is: {}\n\nbase dir: {}\n", line_no, line,
                                     paragraph_dir == 0 ? "LTR" : paragraph_dir == 1 ? "RTL" : "AUTO");
            std::cerr << "expected levels:" << join(expected_levels, true) << "\n";
            std::cerr << "returned levels:" << join(levels) << "\n";
            std::cerr << "expected order:" << join(expected_ltor) << "\n";
            std::cerr << "returned order:" << join(order) << "\n";
            if (debug) {
                mcpplibs::mwrap::fribidi::fribidi_set_debug(1);
                if (fribidi_get_par_embedding_levels_ex(types.data(), bracket_types.data(), len, &base_dir, levels.data())) {
                }
                mcpplibs::mwrap::fribidi::fribidi_set_debug(0);
            }
            std::cerr << "\n";
        }
    }

    if (numerrs)
        std::cerr << std::format("{} errors\n", numerrs);
    else
        std::cout << "No errors found! :-)\n";
    std::cout << std::format("BidiCharacterTest: {} tests, {} errors\n", numtests, numerrs);
    return numerrs ? 1 : 0;
}

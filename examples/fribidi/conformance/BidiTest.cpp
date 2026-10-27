// BidiTest — FriBidi's runner of the Unicode conformance file BidiTest.txt
// (test/unicode-conformance/BidiTest.c of the 1.0.16 release), written
// against `import mcpplibs.mwrap.fribidi;`.
//
// Each test line is a sequence of bidi classes and a bit set of paragraph
// directions (1 auto, 2 LTR, 4 RTL); @Levels and @Reorder give the expected
// levels (x: removed by rule X9) and visual order of the lines that follow.
// As upstream's runner: levels from fribidi_get_par_embedding_levels_ex
// without bracket types (the file has no characters), the order from
// fribidi_reorder_line with no flags, X9-removed classes dropped from it.
//
//   BidiTest [--debug] BidiTest.txt
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

FriBidiCharType parse_char_type(std::string_view s) {
    static const std::pair<std::string_view, FriBidiCharType> names[] = {
        {"L", FRIBIDI_TYPE_LTR},   {"R", FRIBIDI_TYPE_RTL},   {"AL", FRIBIDI_TYPE_AL},   {"EN", FRIBIDI_TYPE_EN},
        {"AN", FRIBIDI_TYPE_AN},   {"ES", FRIBIDI_TYPE_ES},   {"ET", FRIBIDI_TYPE_ET},   {"CS", FRIBIDI_TYPE_CS},
        {"NSM", FRIBIDI_TYPE_NSM}, {"BN", FRIBIDI_TYPE_BN},   {"B", FRIBIDI_TYPE_BS},    {"S", FRIBIDI_TYPE_SS},
        {"WS", FRIBIDI_TYPE_WS},   {"ON", FRIBIDI_TYPE_ON},   {"LRE", FRIBIDI_TYPE_LRE}, {"RLE", FRIBIDI_TYPE_RLE},
        {"LRO", FRIBIDI_TYPE_LRO}, {"RLO", FRIBIDI_TYPE_RLO}, {"PDF", FRIBIDI_TYPE_PDF}, {"LRI", FRIBIDI_TYPE_LRI},
        {"RLI", FRIBIDI_TYPE_RLI}, {"FSI", FRIBIDI_TYPE_FSI}, {"PDI", FRIBIDI_TYPE_PDI},
    };
    for (auto [name, type] : names)
        if (name == s) return type;
    die("Oops. I shouldn't reach here!\n");
}

// Integers as strtol reads them, `x` as -1 when allowed.
std::vector<int> parse_numbers(std::string_view s, bool x_allowed) {
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
        auto [end, ec] = std::from_chars(s.data() + i, s.data() + s.size(), value);
        if (ec != std::errc{}) {
            if (x_allowed) die("Oops. I shouldn't be here!\n");
            break;
        }
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
    if (argc < 2) die(std::format("usage: {} [--debug] test-file-name\n", argv[0]));
    int next_arg = 1;
    bool debug = false;
    if (std::string_view(argv[next_arg]) == "--debug") {
        debug = true;
        next_arg++;
    }
    if (next_arg >= argc) die(std::format("usage: {} [--debug] test-file-name\n", argv[0]));
    const char* filename = argv[next_arg];
    std::ifstream channel(filename, std::ios::binary);
    if (!channel) die(std::format("Failed opening {}\n", filename));

    std::vector<int> expected_levels, expected_ltor;
    int numerrs = 0, numtests = 0, line_no = 0;
    std::string line;
    while (std::getline(channel, line)) {
        line_no++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '@') {
            if (line.starts_with("@Reorder:")) expected_ltor = parse_numbers(std::string_view(line).substr(9), false);
            else if (line.starts_with("@Levels:")) expected_levels = parse_numbers(std::string_view(line).substr(8), true);
            continue;
        }

        // A test line: bidi classes, `;`, the paragraph directions to test.
        std::vector<FriBidiCharType> types;
        std::size_t i = 0;
        for (;;) {
            while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
            std::size_t end = i;
            while (end < line.size() && std::isalpha(static_cast<unsigned char>(line[end]))) ++end;
            if (end == i) break;
            types.push_back(parse_char_type(std::string_view(line).substr(i, end - i)));
            i = end;
        }
        if (i < line.size() && line[i] == ';') ++i;
        const int base_dir_flags = std::atoi(line.c_str() + i);
        const auto types_len = static_cast<FriBidiStrIndex>(types.size());
        std::vector<FriBidiLevel> levels(types.size());
        std::vector<FriBidiStrIndex> ltor(types.size());

        for (int base_dir_mode = 0; base_dir_mode < 3; base_dir_mode++) {
            if ((base_dir_flags & (1 << base_dir_mode)) == 0) continue;
            numtests++;
            const FriBidiParType requested = base_dir_mode == 0 ? FRIBIDI_PAR_ON : base_dir_mode == 1 ? FRIBIDI_PAR_LTR : FRIBIDI_PAR_RTL;
            FriBidiParType base_dir = requested;
            if (fribidi_get_par_embedding_levels_ex(types.data(), nullptr, types_len, &base_dir, levels.data())) {
            }
            std::iota(ltor.begin(), ltor.end(), 0);
            if (fribidi_reorder_line(0, types.data(), types_len, 0, base_dir, levels.data(), nullptr, ltor.data())) {
            }
            std::vector<int> order;
            for (auto index : ltor)
                if (!FRIBIDI_IS_EXPLICIT_OR_BN(types[static_cast<std::size_t>(index)])) order.push_back(index);

            bool matches = levels.size() == expected_levels.size();
            for (std::size_t k = 0; matches && k < levels.size(); ++k)
                if (levels[k] != expected_levels[k] && expected_levels[k] != -1) matches = false;
            if (order != expected_ltor) matches = false;

            if (!matches) {
                numerrs++;
                std::cerr << std::format("failure on line {}\ninput is: {}\n\nbase dir: {}\n", line_no, line,
                                         base_dir_mode == 0 ? "auto" : base_dir_mode == 1 ? "LTR" : "RTL");
                std::cerr << "expected levels:" << join(expected_levels, true) << "\n";
                std::cerr << "returned levels:" << join(levels) << "\n";
                std::cerr << "expected order:" << join(expected_ltor) << "\n";
                std::cerr << "returned order:" << join(order) << "\n";
                if (debug) {
                    mcpplibs::mwrap::fribidi::fribidi_set_debug(1);
                    FriBidiParType again = requested;
                    if (fribidi_get_par_embedding_levels_ex(types.data(), nullptr, types_len, &again, levels.data())) {
                    }
                    mcpplibs::mwrap::fribidi::fribidi_set_debug(0);
                }
                std::cerr << "\n";
            }
        }
    }

    if (numerrs)
        std::cerr << std::format("{} errors out of {} total tests\n", numerrs, numtests);
    else
        std::cout << "No errors found! :-)\n";
    std::cout << std::format("BidiTest: {} tests, {} errors\n", numtests, numerrs);
    return numerrs ? 1 : 0;
}

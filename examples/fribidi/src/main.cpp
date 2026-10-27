// fribidi — GNU FriBidi's command line program (bin/fribidi-main.c of the
// 1.0.16 release), written against `import mcpplibs.mwrap.fribidi;`: it
// converts each logical line of its input to visual order, in any of the
// library's character sets. Its options, output and exit status are
// upstream's, so upstream's charset tests (test/*.input against
// *.reference, `fribidi --test --charset CS`) run on it unchanged.
//
// Option parsing follows getopt_long, as upstream's does: options and files
// in any order, `--name=value` or `--name value`, unique prefixes of long
// names, clustered short options, `--` ends the options, `-` is stdin.
import std;
import mcpplibs.mwrap.fribidi;

using namespace mcpplibs::mwrap::fribidi::macros;  // FRIBIDI_PAR_*, FRIBIDI_IS_RTL, FRIBIDI_NAME …

namespace {

constexpr std::string_view appname = "fribidi";
constexpr int max_str_len = 65000;
constexpr int default_text_width = 80;

struct Options {
    bool do_break = true, do_pad = true, do_mirror = true, do_reorder_nsm = false, do_clean = false;
    bool show_input = false, show_visual = true, show_basedir = false;
    bool show_ltov = false, show_vtol = false, show_levels = false;
    int text_width = default_text_width;
    std::string char_set = "UTF-8";
    std::optional<std::string> bol_text, eol_text;
    FriBidiParType input_base_direction = FRIBIDI_PAR_ON;
};

void out(std::string_view s) { std::cout.write(s.data(), static_cast<std::streamsize>(s.size())); }

[[noreturn]] void die2(std::optional<std::string_view> message) {
    std::cout.flush();
    std::cerr << appname << ": ";
    if (message) std::cerr << *message;
    std::cerr << "Try `" << appname << " --help' for more information.\n";
    std::exit(-1);
}

// printf("%*s", width, s): a negative width left-justifies, as printf's does.
void padded(long long width, std::string_view s) {
    const auto size = static_cast<long long>(s.size());
    if (width >= 0) {
        out(std::string(static_cast<std::size_t>(std::max(0LL, width - size)), ' '));
        out(s);
    } else {
        out(s);
        out(std::string(static_cast<std::size_t>(std::max(0LL, -width - size)), ' '));
    }
}

[[noreturn]] void help(const Options& o) {
    out(std::format("Usage: {0} [OPTION]... [FILE]...\n"
                    "A command line interface for the {1} library.\n"
                    "Convert a logical string to visual.\n"
                    "\n"
                    "  -h, --help            Display this information and exit\n"
                    "  -V, --version         Display version information and exit\n"
                    "  -v, --verbose         Verbose mode, same as --basedir --ltov --vtol\n"
                    "                        --levels\n",
                    appname, FRIBIDI_NAME));
    out(std::format("  -d, --debug           Output debug information\n"
                    "  -t, --test            Test {}, same as --clean --nobreak\n"
                    "                        --showinput --reordernsm --width {}\n",
                    FRIBIDI_NAME, default_text_width));
    out(std::format("  -c, --charset CS      Specify character set, default is {}\n"
                    "      --charsetdesc CS  Show descriptions for character set CS and exit\n"
                    "      --caprtl          Old style: set character set to CapRTL\n",
                    o.char_set));
    out(std::format("      --showinput       Output the input string too\n"
                    "      --nopad           Do not right justify RTL lines\n"
                    "      --nobreak         Do not break long lines\n"
                    "  -w, --width W         Screen width for padding, default is {}, but if\n"
                    "                        environment variable COLUMNS is defined, its value\n"
                    "                        will be used, --width overrides both of them.\n",
                    default_text_width));
    out("  -B, --bol BOL         Output string BOL before the visual string\n"
        "  -E, --eol EOL         Output string EOL after the visual string\n"
        "      --rtl             Force base direction to RTL\n"
        "      --ltr             Force base direction to LTR\n"
        "      --wrtl            Set base direction to RTL if no strong character found\n");
    out("      --wltr            Set base direction to LTR if no strong character found\n"
        "                        (default)\n"
        "      --nomirror        Turn mirroring off, to do it later\n"
        "      --reordernsm      Reorder NSM sequences to follow their base character\n"
        "      --clean           Remove explicit format codes in visual string\n"
        "                        output, currently does not affect other outputs\n"
        "      --basedir         Output Base Direction\n");
    out("      --ltov            Output Logical to Visual position map\n"
        "      --vtol            Output Visual to Logical position map\n"
        "      --levels          Output Embedding Levels\n"
        "      --novisual        Do not output the visual string, to be used with\n"
        "                        --basedir, --ltov, --vtol, --levels\n");
    out("  All string indexes are zero based\n"
        "\n"
        "Output:\n"
        "  For each line of input, output something like this:\n"
        "    [input-str` => '][BOL][[padding space]visual-str][EOL]\n"
        "    [\\n base-dir][\\n ltov-map][\\n vtol-map][\\n levels]\n");
    out("\nAvailable character sets:\n");
    for (int i = 1; i <= FRIBIDI_CHAR_SETS_NUM; i++) {
        const auto set = static_cast<FriBidiCharSet>(i);
        out(std::format("  * {:<10}: {:<25}{:>1}\n", fribidi_char_set_name(set), fribidi_char_set_title(set),
                        fribidi_char_set_desc(set) ? "X" : ""));
    }
    out("  X: Character set has descriptions, use --charsetdesc to see\n");
    out(std::format("\nReport bugs online at\n<{}>.\n", FRIBIDI_BUGREPORT));
    std::exit(0);
}

[[noreturn]] void version() {
    out(std::format("{} {}", appname, fribidi_version_info));
    std::exit(0);
}

[[noreturn]] void charset_description(const std::string& name) {
    const FriBidiCharSet set = fribidi_parse_charset(name.c_str());
    if (!set) die2(std::format("unrecognized character set `{}'\n", name));
    if (!fribidi_char_set_desc(set))
        die2(std::format("no description available for character set `{}'\n", fribidi_char_set_name(set)));
    out(std::format("Descriptions for character set {}:\n\n{}", fribidi_char_set_title(set), fribidi_char_set_desc(set)));
    std::exit(0);
}

// getopt_long's grammar over upstream's option table.
struct LongOption {
    std::string_view name;
    bool argument;
    char32_t id;  // the short option, or a private code
};
enum : char32_t { CHARSETDESC = 257, CAPRTL, SHOWINPUT, NOPAD, NOBREAK, NOMIRROR, REORDERNSM, CLEAN, LTR, RTL, WLTR, WRTL,
                  BASEDIR, LTOV, VTOL, LEVELS, NOVISUAL };
constexpr LongOption long_options[] = {
    {"help", false, 'h'},        {"version", false, 'V'},     {"verbose", false, 'v'},       {"debug", false, 'd'},
    {"test", false, 't'},        {"charset", true, 'c'},      {"charsetdesc", true, CHARSETDESC},
    {"caprtl", false, CAPRTL},   {"showinput", false, SHOWINPUT}, {"nopad", false, NOPAD},
    {"nobreak", false, NOBREAK}, {"width", true, 'w'},        {"bol", true, 'B'},            {"eol", true, 'E'},
    {"nomirror", false, NOMIRROR}, {"reordernsm", false, REORDERNSM}, {"clean", false, CLEAN},
    {"ltr", false, LTR},         {"rtl", false, RTL},         {"wltr", false, WLTR},         {"wrtl", false, WRTL},
    {"basedir", false, BASEDIR}, {"ltov", false, LTOV},       {"vtol", false, VTOL},         {"levels", false, LEVELS},
    {"novisual", false, NOVISUAL},
};
constexpr std::string_view short_options = "hVvdtc:w:B:E:";

void apply(Options& o, char32_t id, const std::string& arg) {
    switch (id) {
    case 'h': help(o);
    case 'V': version();
    case 'v': o.show_basedir = o.show_ltov = o.show_vtol = o.show_levels = true; break;
    case 'w':
        o.text_width = std::atoi(arg.c_str());
        if (o.text_width <= 0) die2(std::format("invalid screen width `{}'\n", arg));
        break;
    case 'B': o.bol_text = arg; break;
    case 'E': o.eol_text = arg; break;
    case 'd':
        // fribidi-common.h declares it in C++ linkage; the module's adaptation links.
        if (!mcpplibs::mwrap::fribidi::fribidi_set_debug(true))
            die2(std::format("lib{} must be compiled with DEBUG option to enable\nturn debug info on.\n", FRIBIDI));
        break;
    case 't':
        o.do_clean = o.show_input = o.do_reorder_nsm = true;
        o.do_break = false;
        o.text_width = default_text_width;
        break;
    case 'c': o.char_set = arg; break;
    case CAPRTL: o.char_set = "CapRTL"; break;
    case CHARSETDESC: charset_description(arg);
    case SHOWINPUT: o.show_input = true; break;
    case NOPAD: o.do_pad = false; break;
    case NOBREAK: o.do_break = false; break;
    case NOMIRROR: o.do_mirror = false; break;
    case REORDERNSM: o.do_reorder_nsm = true; break;
    case CLEAN: o.do_clean = true; break;
    case LTR: o.input_base_direction = FRIBIDI_PAR_LTR; break;
    case RTL: o.input_base_direction = FRIBIDI_PAR_RTL; break;
    case WLTR: o.input_base_direction = FRIBIDI_PAR_WLTR; break;
    case WRTL: o.input_base_direction = FRIBIDI_PAR_WRTL; break;
    case BASEDIR: o.show_basedir = true; break;
    case LTOV: o.show_ltov = true; break;
    case VTOL: o.show_vtol = true; break;
    case LEVELS: o.show_levels = true; break;
    case NOVISUAL: o.show_visual = false; break;
    default: break;
    }
}

std::vector<std::string> parse(Options& o, std::span<char*> argv) {
    std::vector<std::string> files;
    for (std::size_t i = 1; i < argv.size(); ++i) {
        const std::string_view a = argv[i];
        if (a == "--") {
            for (++i; i < argv.size(); ++i) files.emplace_back(argv[i]);
            break;
        }
        if (a.starts_with("--")) {
            const auto eq = a.find('=');
            const std::string_view name = a.substr(2, eq == std::string_view::npos ? std::string_view::npos : eq - 2);
            const LongOption* match = nullptr;
            std::vector<const LongOption*> prefixed;
            for (const auto& option : long_options) {
                if (option.name == name) {
                    match = &option;
                    prefixed = {&option};
                    break;
                }
                if (option.name.starts_with(name)) {
                    match = &option;
                    prefixed.push_back(&option);
                }
            }
            if (prefixed.size() != 1) {
                std::cout.flush();
                if (prefixed.empty()) {
                    std::cerr << appname << ": unrecognized option '" << a << "'\n";
                } else {
                    std::cerr << appname << ": option '" << a << "' is ambiguous; possibilities:";
                    for (const auto* option : prefixed) std::cerr << " '--" << option->name << "'";
                    std::cerr << "\n";
                }
                die2(std::nullopt);
            }
            std::string arg;
            if (match->argument) {
                if (eq != std::string_view::npos)
                    arg = a.substr(eq + 1);
                else if (i + 1 < argv.size())
                    arg = argv[++i];
                else {
                    std::cerr << appname << ": option '--" << match->name << "' requires an argument\n";
                    die2(std::nullopt);
                }
            } else if (eq != std::string_view::npos) {
                std::cerr << appname << ": option '--" << match->name << "' doesn't allow an argument\n";
                die2(std::nullopt);
            }
            apply(o, match->id, arg);
            continue;
        }
        if (a.size() > 1 && a[0] == '-') {
            for (std::size_t k = 1; k < a.size(); ++k) {
                const auto at = short_options.find(a[k]);
                if (a[k] == ':' || at == std::string_view::npos) {
                    std::cerr << appname << ": invalid option -- '" << a[k] << "'\n";
                    die2(std::nullopt);
                }
                if (at + 1 < short_options.size() && short_options[at + 1] == ':') {
                    std::string arg;
                    if (k + 1 < a.size())
                        arg = a.substr(k + 1);
                    else if (i + 1 < argv.size())
                        arg = argv[++i];
                    else {
                        std::cerr << appname << ": option requires an argument -- '" << a[k] << "'\n";
                        die2(std::nullopt);
                    }
                    apply(o, static_cast<unsigned char>(a[k]), arg);
                    break;
                }
                apply(o, static_cast<unsigned char>(a[k]), {});
            }
            continue;
        }
        files.emplace_back(a);
    }
    return files;
}

// One line of input, as upstream's loop body handles it.
int convert(const Options& o, FriBidiCharSet char_set_num, std::string line, bool newline, int padding_width, int break_width) {
    std::string_view nl_found = "";
    std::vector<FriBidiChar> logical(max_str_len);
    auto len = fribidi_charset_to_unicode(char_set_num, line.data(), static_cast<FriBidiStrIndex>(line.size()), logical.data());

    const auto n = static_cast<std::size_t>(len) + 1;
    std::vector<FriBidiChar> visual(o.show_visual ? n : 0);
    std::vector<FriBidiStrIndex> ltov(o.show_ltov ? n : 0), vtol(o.show_vtol ? n : 0);
    std::vector<FriBidiLevel> levels(o.show_levels ? n : 0);
    auto data = [](auto& v) { return v.empty() ? nullptr : v.data(); };

    FriBidiParType base = o.input_base_direction;
    const FriBidiLevel log2vis = fribidi_log2vis(logical.data(), len, &base, data(visual), data(ltov), data(vtol), data(levels));
    if (!log2vis) return 2;

    if (o.show_input) {
        out(line);
        out(std::string(static_cast<std::size_t>(std::max<long long>(0, padding_width - static_cast<long long>(line.size()))), ' '));
        out(" => ");
    }
    if (o.do_clean) len = fribidi_remove_bidi_marks(data(visual), len, data(ltov), data(vtol), data(levels));

    if (o.show_visual) {
        out(nl_found);
        if (o.bol_text) out(*o.bol_text);
        std::string outstring(static_cast<std::size_t>(max_str_len), '\0');
        for (FriBidiStrIndex idx = 0; idx < len;) {
            FriBidiStrIndex wid = break_width;
            const FriBidiStrIndex st = idx;
            if (char_set_num != FRIBIDI_CHAR_SET_CAP_RTL)
                while (wid > 0 && idx < len) {
                    wid -= FRIBIDI_IS_EXPLICIT_OR_ISOLATE_OR_BN_OR_NSM(fribidi_get_bidi_type(visual[static_cast<std::size_t>(idx)])) ? 0 : 1;
                    idx++;
                }
            else
                while (wid > 0 && idx < len) {
                    wid--;
                    idx++;
                }
            if (wid < 0 && idx - st > 1) idx--;
            const FriBidiStrIndex inlen = idx - st;

            const FriBidiStrIndex bytes = fribidi_unicode_to_charset(char_set_num, visual.data() + st, inlen, outstring.data());
            const std::string_view piece(outstring.data(), static_cast<std::size_t>(bytes));
            if (FRIBIDI_IS_RTL(base))
                // (int)(padding_width + strlen(outstring) - (break_width - wid)), as upstream computes it.
                padded(o.do_pad ? static_cast<int>(static_cast<std::size_t>(padding_width) + piece.size() -
                                                   static_cast<std::size_t>(break_width - wid))
                                : 0,
                       piece);
            else
                out(piece);
            if (idx < len) out("\n");
        }
        if (o.eol_text) out(*o.eol_text);
        nl_found = "\n";
    }
    if (o.show_basedir) {
        out(nl_found);
        out(std::format("Base direction: {}", FRIBIDI_DIR_TO_LEVEL(base) ? "R" : "L"));
        nl_found = "\n";
    }
    auto list = [&](const auto& values) {
        out(nl_found);
        for (FriBidiStrIndex i = 0; i < len; i++) out(std::format("{} ", static_cast<long>(values[static_cast<std::size_t>(i)])));
        nl_found = "\n";
    };
    if (o.show_ltov) list(ltov);
    if (o.show_vtol) list(vtol);
    if (o.show_levels) list(levels);
    if (!nl_found.empty()) out(newline ? "\n" : "");
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    Options o;
    if (const char* columns = std::getenv("COLUMNS")) {
        const int i = std::atoi(columns);
        if (i > 0) o.text_width = i;
    }
    const auto files = parse(o, std::span(argv, static_cast<std::size_t>(argc)));

    const FriBidiCharSet char_set_num = fribidi_parse_charset(o.char_set.c_str());
    if (!char_set_num) die2(std::format("unrecognized character set `{}'\n", o.char_set));

    // Upstream sets the library's (deprecated) global flags too.
    fribidi_set_mirroring(o.do_mirror);
    fribidi_set_reorder_nsm(o.do_reorder_nsm);

    const int padding_width = o.show_input ? (o.text_width - 10) / 2 : o.text_width;
    const int break_width = o.do_break ? padding_width : 3 * max_str_len;

    int exit_val = 0;
    const std::vector<std::string> inputs = files.empty() ? std::vector<std::string>{"-"} : files;
    for (const auto& filename : inputs) {
        std::ifstream file;
        std::istream* in = &std::cin;
        if (filename != "-") {
            file.open(filename, std::ios::binary);
            if (!file) {
                std::cout.flush();
                std::cerr << appname << ": " << filename << ": no such file or directory\n";
                exit_val = 1;
                continue;
            }
            in = &file;
        }
        // fgets into a buffer of max_str_len - 1 bytes: a longer line is read in pieces.
        std::string line;
        char c;
        bool any = false;
        auto flush = [&](bool newline) {
            if (const int status = convert(o, char_set_num, line, newline, padding_width, break_width)) exit_val = status;
            line.clear();
            any = false;
        };
        while (in->get(c)) {
            any = true;
            if (c == '\n') {
                flush(true);
                continue;
            }
            line.push_back(c);
            if (line.size() == static_cast<std::size_t>(max_str_len - 2)) flush(false);
        }
        if (any) flush(false);
    }
    std::cout.flush();
    return exit_val;
}

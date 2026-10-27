# fribidi

> 以 C++23 模块使用 GNU FriBidi——Unicode 双向文本算法。

[English](README.md) · **简体中文** · [Upstream](https://github.com/fribidi/fribidi) · [Releases](https://github.com/mcpplibs/fribidi-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![FriBidi](https://img.shields.io/badge/FriBidi-LGPL--2.1--or--later-blue.svg)](upstream/fribidi/COPYING)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/fribidi-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/fribidi-m/actions/workflows/ci.yml)

FriBidi 把按逻辑顺序存储的文本转换成显示所需的视觉顺序，用于阿拉伯文、
希伯来文和混合方向文本（UAX #9，Unicode 16.0.0）。上游 C API 保持不变：
`FriBidiCharType`、`fribidi_get_par_embedding_levels_ex`、
`fribidi_reorder_line`、`fribidi_log2vis` 都是 `<fribidi.h>` 的上游全局名字。
库本身是 `compat.fribidi` 1.0.16。本包把 FriBidi 的宏映射为常量和 constexpr
函数，并逐一与头文件对照检查。FriBidi 自己的测试套件在每次 CI 中全部通过：
字符集测试和全部 861,948 个 Unicode 一致性用例，运行在基于模块编写的程序上。

## 模块

- `import mcpplibs.mwrap.fribidi;`：`<fribidi.h>` 的完整 API，并重新导出下面的模块
- `import mcpplibs.mwrap.fribidi.macros;`：`FRIBIDI_PAR_ON`、`FRIBIDI_TYPE_RTL`、`FRIBIDI_FLAGS_DEFAULT`、`FRIBIDI_IS_RTL` 等，位于 `mcpplibs::mwrap::fribidi::macros`

> 更多模块 API -> [用法与 API](docs/usage.md)

## 快速开始

```sh
mcpp new bidi --template mcpplibs.mwrap.fribidi@1.0.16
cd bidi
mcpp run
```

默认模板按渲染器的方式排版 UTF-8 文本行：先计算双向类型、括号配对和嵌入层级，
再把每一行重排为视觉顺序并镜像括号。用 `mcpp run -- "你的文本"` 处理自己的文本。

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

## 宏与两个适配函数

`FRIBIDI_TYPE_LTR` 这类常量保留宏在 C++ 中的类型（`long`）。`FRIBIDI_IS_RTL`、
`FRIBIDI_SET_BITS` 这类计算宏是 constexpr 函数，函数体就是宏本身；对任何参数类型，
结果的值、类型和值类别都与宏一致。属性、`extern "C"` 括号、pragma 和 X-macro
表仍是宏，放在 `<mwrap/fribidi/macros.hpp>` 中，在 import 之前包含。

`fribidi-common.h` 把 `fribidi_debug_status` 和 `fribidi_set_debug` 声明在
`extern "C"` 块之外，所以 C++ 对它们的调用无法链接。模块在
`mcpplibs::mwrap::fribidi` 中适配了这两个函数。详见 [docs/usage.md](docs/usage.md)。

## 示例

- [examples/fribidi](examples/fribidi)：上游的 `fribidi` 命令行程序（逻辑顺序转视觉顺序，支持它的全部字符集）
  及其 Unicode 一致性测试运行器，均基于模块编写；[tools/run_conformance.py](tools/run_conformance.py) 在其上运行 FriBidi 的测试。

## 构建与测试

Linux、macOS 和 Windows CI 构建模块，运行测试和导出审计
（[tools/check_exports.py](tools/check_exports.py)）。审计检查生成的源码是否最新、
每个声明是否都已导出、每个宏是否都已映射或注明保留原因。CI 还运行 FriBidi 的测试套件：
六个字符集测试、BidiTest.txt（770,241 个用例）和 BidiCharacterTest.txt（91,707 个用例）。
另一个任务把默认模板当作已安装的包创建项目并运行。

## 许可证

封装代码采用 [Apache-2.0](LICENSE)，作者为 mcpp-community（[NOTICE](NOTICE)）。
GNU FriBidi 仍为 LGPL-2.1-or-later（[upstream/fribidi/COPYING](upstream/fribidi/COPYING)，
[作者](upstream/fribidi/AUTHORS)）。它以静态方式链接，因此以二进制形式分发的程序必须满足
LGPL 对 FriBidi 的要求，包括允许重新链接（[docs/usage.md](docs/usage.md#licensing)）。
其字符表来自 Unicode 字符数据库（[声明](upstream/fribidi/gen.tab/unidata/ReadMe.txt)）。
[examples/fribidi](examples/fribidi) 中的移植程序仍为 LGPL-2.1-or-later。

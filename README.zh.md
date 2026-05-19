# freq - 纯 Vibe 编程语言

> **freq** 是一门高性能、跨平台的编程语言，专为**纯 Vibe 编程**设计——让编写代码变得自然流畅，执行速度飞快。


---

## ✨ 纯 Vibe 编程特性

### 什么是"纯 Vibe 编程"？
纯 Vibe 编程意味着编写的代码**感觉自然**。没有繁琐的样板代码，没有多余的仪式感，只有简洁、表达力强的语法，让你专注于解决问题。

### 核心 Vibe 原则
1. **零仪式感** - 代码读起来就像你的思考过程
2. **即时反馈** - JIT 模式支持快速开发循环
3. **原生性能** - 编译为优化的机器码
4. **安全优先** - 内置内存安全检查
5. **嵌入式设计** - 极小运行时，完美适配嵌入式系统

---

## 🚀 快速开始

```bash
# 编译编译器
cd freq
make

# JIT 模式运行（即时反馈）
./freq -r test/example.fq

# 编译模式（生成可执行文件）
./freq test/example.fq -o myapp
./myapp
```

### Hello World 示例
```freq
func main() {
    printn("Hello, Pure Vibe Coding!")
}
```

---

## 📦 V0.1 功能特性

### 语言特性
- ✅ **类 C 语法** - 熟悉且易于学习
- ✅ **多种变量类型** - `var`, `let`, `const`
- ✅ **丰富的类型系统** - `int`, `int64`, `float`, `double`, `bool`, `string`, `char`
- ✅ **强大的函数系统** - C 风格和 Go 风格参数语法
- ✅ **数组** - 多维数组支持，带 `.len` 属性
- ✅ **结构体** - 自定义数据结构
- ✅ **枚举** - 支持自动递增和手动赋值
- ✅ **指针** - 内存地址操作
- ✅ **类型转换** - 多种转换风格
- ✅ **控制流程** - `if-elif-else`, `switch`, `while`, `for`, `fo`
- ✅ **循环控制** - `break`, `continue`, `quit`, `exit`

### 模块化系统
- ✅ **模块声明** - `mod` 关键字
- ✅ **导入整个模块** - `import module`
- ✅ **导入特定成员** - `use module::function()`
- ✅ **导出变量** - `out var` 关键字
- ✅ **模块访问** - `::` 操作符

### 编译特性
- ✅ **LLVM 后端** - 直接生成 LLVM IR
- ✅ **JIT 模式** - 即时执行，适合开发
- ✅ **编译模式** - 优化的原生可执行文件
- ✅ **性能指标** - 内置时间统计

### 内存安全
- ✅ **未初始化变量检测**
- ✅ **空指针解引用检测**
- ✅ **数组边界检查**

---

## ⚠️ V0.1 缺失功能

### 标准库
- ❌ 无文件系统操作
- ❌ 无网络支持
- ❌ 无图形库
- ❌ 数学函数有限

### 并发支持
- ❌ 无多线程
- ❌ 无协程
- ❌ 无 async/await

### 工具链
- ❌ 无调试器
- ❌ 无包管理器
- ❌ 无 IDE 集成

### 平台支持
- ❌ Windows GUI 支持
- ❌ ARM 单片机支持
- ❌ WebAssembly 目标

---

## 📋 V0.2 路线图

### 计划功能
1. **标准库扩展**
   - 文件 I/O 操作
   - 基础网络支持
   - 数学库
   - 字符串工具

2. **并发支持**
   - 基础线程
   - 原子操作
   - 线程安全数据结构

3. **调试工具**
   - 基础调试器集成
   - 堆栈跟踪
   - 断点支持

4. **平台改进**
   - 更好的 Windows 支持
   - ARM64 优化
   - 初步 WASM 目标

5. **语言增强**
   - Lambda 函数
   - 泛型
   - 错误处理

### 目标时间表
- Q2 2026: 标准库
- Q3 2026: 并发支持
- Q4 2026: 调试器 & 平台扩展

---

## 🎯 性能

### 基准测试结果（100万次迭代）

| 操作 | JIT 模式 | 编译模式 |
|------|----------|----------|
| 循环求和 | ~4ms | ~0.37ms |
| 数组访问 | ~5ms | ~0.4ms |
| 函数调用 | ~6ms | ~0.5ms |

### 性能特性
- LLVM 优化管道
- JIT 缓存支持快速重运行
- 零成本抽象
- 极小运行时开销

---

## 📁 项目结构

```
freq/
├── src/          # 编译器源代码
│   ├── main.c    # 入口点和预处理器
│   ├── lexer.c   # 词法分析器
│   ├── parser.c  # AST 解析器
│   └── codegen.c # LLVM 代码生成
├── include/      # 头文件
├── test/         # 测试程序
├── README.md     # 项目介绍（英文）
├── README.zh.md  # 项目介绍（中文）
├── FEATURES.md   # 功能特性（英文）
├── FEATURES.zh.md # 功能特性（中文）
├── TUTORIAL.md   # 完整教程（英文）
├── TUTORIAL.zh.md # 完整教程（中文）
└── Makefile      # 构建配置
```

---

## 🛠️ 构建要求

- LLVM 20+
- GCC/Clang 编译器
- POSIX 兼容系统（Linux/macOS）

```bash
# macOS 安装 LLVM
brew install llvm

# Ubuntu 安装 LLVM
sudo apt-get install llvm-20 llvm-20-dev
```

---

## 📝 使用示例

### 模块系统
```freq
# math.fq
mod math
out var PI = 3.14
func add(a:int, b:int) { return a + b : int }
```

```freq
# main.fq
import math
use math::add

func main() {
    printn(math::PI)    # 3.14
    printn(add(2, 3))   # 5
}
```

### 游戏开发
```freq
struct Player {
    x: int
    y: int
    score: int
}

func update(player:*Player) {
    player->x += 1
    player->score += 10
}
```

---

## 🤝 贡献

欢迎贡献！以下是你可以提供帮助的方式：

1. **报告 Bug** - 为 bug 或意外行为打开 issue
2. **编写测试** - 为新功能添加测试用例
3. **完善文档** - 帮助文档化语言
4. **编写代码** - 实现路线图中的功能

### 开发流程
```bash
# 克隆仓库
git clone https://github.com/yourusername/freq.git
cd freq

# 构建
make

# 测试
./freq -r test/simple.fq

# 调试（带 LLVM 调试信息）
make debug
```

---

## 📄 许可证

MIT License - 详见 LICENSE 文件

---

## 🙋 FAQ

**Q: freq 与其他语言相比有什么优势？**
A: freq 专为嵌入式系统和游戏开发设计，专注于极简主义和性能。"纯 Vibe" 哲学意味着更少的仪式感，更多的问题解决。

**Q: 可以用于 Web 开发吗？**
A: V0.1 暂不支持 WebAssembly，但 V0.2 计划支持。

**Q: freq 适合生产环境吗？**
A: V0.1 主要用于开发和实验。生产环境建议等待 V1.0。

---

*freq - 为现代开发者打造的纯 Vibe 编程语言*

---

**版本:** 0.1.0  
**状态:** 积极开发中  
**最后更新:** 2026年5月

---

## 📖 文档

- **English**: `README.md`, `FEATURES.md`, `TUTORIAL.md`
- **中文**: `README.zh.md`, `FEATURES.zh.md`, `TUTORIAL.zh.md`
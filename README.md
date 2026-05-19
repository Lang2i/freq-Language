# freq - Pure Vibe Coding Language

> **freq** is a high-performance, cross-platform programming language designed for **pure vibe coding** - where writing code feels natural and execution is lightning fast.


---

## ✨ Pure Vibe Coding Features

### What is "Pure Vibe Coding"?
Pure Vibe Coding means writing code that just *feels right*. No boilerplate, no ceremony, just clean, expressive syntax that lets you focus on solving problems.

### Core Vibe Principles
1. **Zero Ceremony** - Write code that reads like your thoughts
2. **Instant Feedback** - JIT mode for rapid development cycles
3. **Native Performance** - Compiles to optimized machine code
4. **Safety First** - Built-in memory safety checks
5. **Embeddable by Design** - Tiny runtime, perfect for embedded systems

---

## 🚀 Quick Start

```bash
# Build the compiler
cd freq
make

# Run in JIT mode (instant feedback)
./freq -r test/example.fq

# Compile to executable (production)
./freq test/example.fq -o myapp
./myapp
```

### Hello World Example
```freq
func main() {
    printn("Hello, Pure Vibe Coding!")
}
```

---

## 📦 V0.1 Features

### Language Features
- ✅ **C-like Syntax** - Familiar and easy to learn
- ✅ **Multiple Variable Types** - `var`, `let`, `const`
- ✅ **Rich Type System** - `int`, `int64`, `float`, `double`, `bool`, `string`, `char`
- ✅ **Powerful Functions** - C-style and Go-style parameter syntax
- ✅ **Arrays** - Multi-dimensional support with `.len` property
- ✅ **Structures** - Custom data structures
- ✅ **Enums** - Enumerated types with auto/increment values
- ✅ **Pointers** - Memory address operations
- ✅ **Type Conversion** - Multiple conversion styles
- ✅ **Control Flow** - `if-elif-else`, `switch`, `while`, `for`, `fo`
- ✅ **Loop Control** - `break`, `continue`, `quit`, `exit`

### Modular System
- ✅ **Module Declaration** - `mod` keyword
- ✅ **Import Whole Module** - `import module`
- ✅ **Import Specific Members** - `use module::function()`
- ✅ **Export Variables** - `out var` keyword
- ✅ **Module Access** - `::` operator

### Compilation
- ✅ **LLVM Backend** - Direct LLVM IR generation
- ✅ **JIT Mode** - Instant execution for development
- ✅ **Compile Mode** - Optimized native executables
- ✅ **Performance Metrics** - Built-in timing statistics

### Memory Safety
- ✅ **Uninitialized Variable Detection**
- ✅ **Null Pointer Dereference Detection**
- ✅ **Array Bounds Checking**

---

## ⚠️ V0.1 Missing Features

### Standard Library
- ❌ No file system operations
- ❌ No network support
- ❌ No graphics library
- ❌ Limited math functions

### Concurrency
- ❌ No multithreading
- ❌ No coroutines
- ❌ No async/await

### Tooling
- ❌ No debugger
- ❌ No package manager
- ❌ No IDE integration

### Platform Support
- ❌ Windows GUI support
- ❌ ARM microcontroller support
- ❌ WebAssembly target

---

## 📋 V0.2 Roadmap

### Planned Features
1. **Standard Library Expansion**
   - File I/O operations
   - Basic networking
   - Math library
   - String utilities

2. **Concurrency Support**
   - Basic threading
   - Atomic operations
   - Thread-safe data structures

3. **Debugging Tools**
   - Basic debugger integration
   - Stack traces
   - Breakpoint support

4. **Platform Improvements**
   - Better Windows support
   - ARM64 optimization
   - Initial WASM target

5. **Language Enhancements**
   - Lambda functions
   - Generics
   - Error handling

### Target Timeline
- Q2 2026: Standard Library
- Q3 2026: Concurrency
- Q4 2026: Debugger & Platforms

---

## 🎯 Performance

### Benchmark Results (1M iterations)

| Operation | JIT Mode | Compiled Mode |
|-----------|----------|---------------|
| Loop + Sum | ~4ms | ~0.37ms |
| Array Access | ~5ms | ~0.4ms |
| Function Call | ~6ms | ~0.5ms |

### Performance Features
- LLVM optimization pipeline
- JIT caching for fast re-runs
- Zero-cost abstractions
- Minimal runtime overhead

---

## 📁 Project Structure

```
freq/
├── src/          # Compiler source code
│   ├── main.c    # Entry point & preprocessor
│   ├── lexer.c   # Lexical analyzer
│   ├── parser.c  # AST parser
│   └── codegen.c # LLVM code generation
├── include/      # Header files
├── test/         # Test programs
├── FEATURES.md   # Detailed feature list (English)
├── FEATURES.zh.md # Detailed feature list (Chinese)
├── TUTORIAL.md   # Complete language tutorial (English)
├── TUTORIAL.zh.md # Complete language tutorial (Chinese)
└── Makefile      # Build configuration
```

---

## 🛠️ Build Requirements

- LLVM 20+
- GCC/Clang compiler
- POSIX-compliant system (Linux/macOS)

```bash
# Install LLVM on macOS
brew install llvm

# Install LLVM on Ubuntu
sudo apt-get install llvm-20 llvm-20-dev
```

---

## 📝 Usage Examples

### Module System
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

### Game Development
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

## 🤝 Contributing

We welcome contributions! Here's how you can help:

1. **Report Bugs** - Open issues for bugs or unexpected behavior
2. **Write Tests** - Add test cases for new features
3. **Improve Docs** - Help document the language
4. **Code** - Implement features from the roadmap

### Development Workflow
```bash
# Clone the repo
git clone https://github.com/yourusername/freq.git
cd freq

# Build
make

# Test
./freq -r test/simple.fq

# Debug (with LLVM debug info)
make debug
```

---

## 📄 License

MIT License - See LICENSE file for details

---

## 🙋 FAQ

**Q: What makes freq different from other languages?**
A: freq is designed specifically for embedded systems and game development with a focus on minimalism and performance. The "pure vibe" philosophy means less ceremony and more focus on solving problems.

**Q: Can I use freq for web development?**
A: V0.1 doesn't support WebAssembly yet, but it's planned for V0.2.

**Q: Is freq suitable for production?**
A: V0.1 is primarily for development and experimentation. For production use, wait for V1.0.

---

*freq - Pure Vibe Coding for the Modern Developer*

---

**Version:** 0.1.0  
**Status:** Active Development  
**Last Updated:** May 2026

---

## 📖 Documentation

- **English**: `README.md`, `FEATURES.md`, `TUTORIAL.md`
- **中文**: `README.zh.md`, `FEATURES.zh.md`, `TUTORIAL.zh.md`
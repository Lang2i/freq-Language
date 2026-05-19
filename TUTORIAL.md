# freq Programming Language - Complete Tutorial

freq is a high-performance, cross-platform programming language designed for **pure vibe coding** - where writing code feels natural and execution is lightning fast.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Variables & Data Types](#variables--data-types)
3. [Operators](#operators)
4. [Control Flow](#control-flow)
5. [Functions](#functions)
6. [Arrays](#arrays)
7. [Structures](#structures)
8. [Enums](#enums)
9. [Pointers](#pointers)
10. [Type Conversion](#type-conversion)
11. [Modularization](#modularization)
12. [Scope](#scope)
13. [Memory Safety](#memory-safety)
14. [Game Development](#game-development)

---

## 1. Quick Start

### 1.1 Installation & Execution

```bash
# Build the compiler
cd freq
make

# Run in JIT mode (instant execution)
./freq -r test/example.fq

# Compile to executable (production)
./freq test/example.fq -o output
./output
```

### 1.2 First Program

```freq
func main() {
    printn("Hello, freq!")
}
```

Output:
```
Running test/example.fq with JIT...
Hello, freq!

  Run: 2.5 ms
  Parse: 0.1 ms
  Compile: 0.5 ms
  Total: 3.1 ms
```

---

## 2. Variables & Data Types

### 2.1 Variable Declaration

freq supports three variable declaration types:

```freq
var x = 10          // Regular mutable variable
let result = get_value()  // One-time constant
const PI = 3.14      // Compile-time constant
```

### 2.2 Data Types

| Type | Keyword | Description | Example |
|------|--------|-------------|---------|
| Integer | `int` | 32-bit signed | `var int age = 25` |
| Long Integer | `int64` | 64-bit signed | `var int64 big = 10000000000` |
| Float | `float` | 32-bit float | `var float pi = 3.14f` |
| Double | `double` | 64-bit float | `var double precise = 3.1415926` |
| Boolean | `bool` | Boolean value | `var bool flag = true` |
| String | `string` | String | `var string name = "freq"` |
| Character | `char` | Single char | `var char c = 'A'` |

### 2.3 Type Annotations

```freq
// Explicit type declaration
var int x = 10
var string name = "Hello"

// Type inference
var y = 20      // int
var z = 3.14    // double
```

---

## 3. Operators

### 3.1 Arithmetic Operators

```freq
var a = 10
var b = 3

printn(a + b)  // 13
printn(a - b)  // 7
printn(a * b)  // 30
printn(a / b)  // 3
```

### 3.2 Comparison Operators

```freq
var x = 5

printn(x == 5)  // true
printn(x != 5)  // false
printn(x > 3)   // true
printn(x < 3)   // false
printn(x >= 5)  // true
printn(x <= 5)  // true
```

### 3.3 Increment/Decrement

```freq
var i = 0
i++  // i = 1
i--  // i = 0
```

---

## 4. Control Flow

### 4.1 if-else

```freq
var score = 85

if (score >= 90) {
    printn("A")
} elif (score >= 80) {
    printn("B")
} elif (score >= 70) {
    printn("C")
} else {
    printn("F")
}
```

### 4.2 switch

```freq
var day = 3

switch (day) {
    1: printn("Monday")
    2: printn("Tuesday")
    3: printn("Wednesday")
    default: printn("Unknown")
}
```

**Multi-value matching:**
```freq
switch (num) {
    1, 3, 5: printn("Odd")
    2, 4, 6: printn("Even")
}
```

**Range matching:**
```freq
switch (age) {
    0..17: printn("Minor")
    18..64: printn("Adult")
    65..120: printn("Senior")
}
```

### 4.3 while Loop

```freq
var i = 0
while (i < 5) {
    printn(i)
    i++
}
```

**while with exit condition:**
```freq
var attempts = 0
while (true) {
    printn("Attempt: ", attempts)
    attempts++
} exit(attempts >= 3)
```

### 4.4 for Loop

```freq
for(var int i = 0; i < 5; i++) {
    printn(i)
}
```

### 4.5 fo Loop (For-Each)

**Array iteration:**
```freq
var arr = [1, 2, 3, 4, 5]
fo (arr) [val] {
    printn(val)
}
```

**Range iteration:**
```freq
fo (1 .. 5) [i] {
    printn(i)
}
```

### 4.6 Loop Control

```freq
// break - Exit loop
for(var int i = 0; i < 10; i++) {
    if (i == 5) break
    printn(i)
}

// continue - Skip iteration
for(var int i = 0; i < 5; i++) {
    if (i == 2) continue
    printn(i)
}

// quit / exit - Exit and continue
while (true) {
    printn("First")
    quit
}
printn("After loop")
```

---

## 5. Functions

### 5.1 Function Definition

**Style 1: C-style (type before name)**
```freq
func add(int a, int b) {
    return a + b : int
}
```

**Style 2: Go-style (type after name)**
```freq
func add(a:int, b:int) {
    return a + b : int
}
```

### 5.2 Function Call

```freq
func main() {
    var result = add(1, 2)
    printn(result)  // 3
}
```

### 5.3 Return Values

```freq
func get_char() {
    return 'A' : char
}

func divide(x:float, y:float) {
    return x / y : float
}
```

### 5.4 Main Function

```freq
func main() {
    // Program entry point
}
```

---

## 6. Arrays

### 6.1 One-dimensional Arrays

```freq
var arr = [1, 2, 3, 4, 5]
printn(arr[0])   // 1
printn(arr[2])   // 3
```

### 6.2 Character Arrays

```freq
var chars = ['a', 'b', 'c']
printn(chars[1])  // b
```

### 6.3 Multi-dimensional Arrays

```freq
var matrix = [[1, 2], [3, 4], [5, 6]]
printn(matrix[0][0])  // 1
printn(matrix[1][1])  // 4
```

### 6.4 Array Properties

```freq
var arr = [1, 2, 3, 4, 5]
var size = arr.len  // 5

for(var int i = 0; i < arr.len; i++) {
    printn(arr[i])
}
```

---

## 7. Structures

### 7.1 Define Structure

```freq
struct Point {
    x: int
    y: int
}
```

### 7.2 Create Instance

```freq
var p = Point {
    x: 10
    y: 20
}
```

### 7.3 Access Members

```freq
printf("x = %d\n", p.x)
printf("y = %d\n", p.y)
```

### 7.4 Modify Members

```freq
p.x = 100
p.y = 200
```

### 7.5 Complex Structures

```freq
struct Sprite {
    x: int
    y: int
    width: int
    height: int
    visible: bool
}

var player = Sprite {
    x: 100
    y: 200
    width: 32
    height: 32
    visible: true
}
```

---

## 8. Enums

### 8.1 Define Enum

```freq
enum Color {
    RED,
    GREEN,
    BLUE
}

enum BaudRate {
    B9600,
    B19200,
    B115200 = 115200  // Manual assignment
}
```

### 8.2 Use Enum

```freq
func main() {
    var c = Color.RED      // 0
    var baud = BaudRate.B115200  // 115200
    printn(Color.GREEN)   // 1
}
```

---

## 9. Pointers

### 9.1 Pointer Declaration

```freq
var ptr:*int  // Pointer to int
```

### 9.2 Get Address

```freq
var x:int = 5
var ptr:*int = &x  // Get address of x
```

### 9.3 Dereference

```freq
var val = *ptr  // Get value
printn(val)     // 5

*ptr = 10       // Modify value
printn(x)       // 10
```

---

## 10. Type Conversion

### 10.1 C-style Conversion

```freq
var int x = (int)3.14
var float y = (float)42
```

### 10.2 Method-style Conversion

```freq
var int x = 3.14.int()
var float y = 42.float()
```

**Supported conversion functions:**
- `int()` → Integer
- `int64()` → Long integer
- `float()` → Float
- `double()` → Double
- `char()` → Character
- `bool()` → Boolean
- `string()` → String

---

## 11. Modularization

### 11.1 Create Module

**math.fq:**
```freq
mod math
out var PI = 3.14
func add(a:int, b:int) {
    return a + b : int
}
func multiply(a:int, b:int) {
    return a * b : int
}
```

### 11.2 Import Module

**Method 1: Import entire module**
```freq
import math

func main() {
    var sum = math::add(1, 2)
    var product = math::multiply(3, 4)
    printn(sum)      // 3
    printn(product)  // 12
}
```

**Method 2: Import specific function (no out needed)**
```freq
use math::add

func main() {
    var sum = add(1, 2)
    printn(sum)  // 3
}
```

**Method 3: Import exported variable (needs out)**
```freq
use math::PI

func main() {
    printn(PI)  // 3.14
}
```

### 11.3 Module System Rules

| Keyword | Purpose | Requires out |
|--------|---------|--------------|
| `mod` | Declare module | - |
| `import` | Import entire module | No |
| `use` | Import specific member | Functions: No / Variables: Yes |
| `out` | Mark variable exportable | - |
| `::` | Module access operator | - |

---

## 12. Scope

### 12.1 Global Variables

```freq
var global_var:int = 100

func main() {
    printn(global_var)  // 100
}
```

### 12.2 Local Variables

```freq
func main() {
    var local_var:int = 50
    printn(local_var)  // 50
}
```

### 12.3 Block Scope

```freq
func main() {
    var a:int = 1
    if (a == 1) {
        var b:int = 2
        printn(b)  // 2
    }
    // printn(b)  // Error: b not visible
}
```

### 12.4 Scope Resolution Rules

1. **Closest first**: Search current scope
2. **Upwards**: Search parent scopes
3. **Global fallback**: Error if not found anywhere

---

## 13. Memory Safety

### 13.1 Uninitialized Variable Detection

```freq
var x:int  // Uninitialized
func main() {
    printn(x)  // Error: Variable 'x' is used without initialization
}
```

### 13.2 Null Pointer Detection

```freq
var ptr:*int
func main() {
    *ptr = 10  // Error: Dereferencing null pointer
}
```

### 13.3 Correct Usage

```freq
var x:int = 5
var ptr:*int = &x

func main() {
    printn(*ptr)  // 5
    *ptr = 10
    printn(x)     // 10
}
```

---

## 14. Game Development

### 14.1 Game Loop

```freq
struct GameState {
    score: int
    lives: int
    frame: int
}

func update_input(state:*GameState) {
    // Handle input
}

func update_game(state:*GameState) {
    state->frame++
}

func render(state:*GameState) {
    printf("Frame: %d, Score: %d\n", state->frame, state->score)
}

func main() {
    var game = GameState {
        score: 0
        lives: 3
        frame: 0
    }
    
    while (game.lives > 0) {
        update_input(&game)
        update_game(&game)
        render(&game)
    }
}
```

### 14.2 Sprite Management

```freq
struct Sprite {
    x: int
    y: int
    width: int
    height: int
    visible: bool
}

var sprites = [Sprite {x:0,y:0,width:32,height:32,visible:true},
               Sprite {x:100,y:50,width:32,height:32,visible:true}]

func update_sprites() {
    fo (sprites) [sprite] {
        if (sprite.visible) {
            sprite.x += 1
        }
    }
}
```

### 14.3 Collision Detection

```freq
struct Rect {
    x: int
    y: int
    width: int
    height: int
}

func check_collision(a:Rect, b:Rect):bool {
    return (a.x < b.x + b.width &&
            a.x + a.width > b.x &&
            a.y < b.y + b.height &&
            a.y + a.height > b.y) : bool
}
```

---

## Appendix: Output Functions

| Function | Description | Example |
|----------|-------------|---------|
| `print()` | Print single value | `print("Hello")` |
| `printn()` | Print with newline | `printn("World")` |
| `printf()` | Formatted print | `printf("Age: %d\n", age)` |
| `printfn()` | Formatted print with newline | `printfn("Name:", name)` |

**printf format specifiers:**
- `%d` - Integer
- `%s` - String
- `%c` - Character
- `%p` - Pointer address
- `%f` - Float

---

## Appendix: Escape Characters

| Escape Sequence | Description |
|----------------|-------------|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\"` | Double quote |

---

*freq Programming Language - Pure Vibe Coding for GUI, Games & Embedded Systems*

**Version:** 0.1.0
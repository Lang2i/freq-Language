# freq 编程语言 - 完整教程

freq 是一门高性能、跨平台的编程语言，专为**纯 Vibe 编程**设计——让编写代码变得自然流畅，执行速度飞快。

## 目录

1. [快速开始](#快速开始)
2. [变量与数据类型](#变量与数据类型)
3. [运算符](#运算符)
4. [控制流程](#控制流程)
5. [函数](#函数)
6. [数组](#数组)
7. [结构体](#结构体)
8. [枚举](#枚举)
9. [指针](#指针)
10. [类型转换](#类型转换)
11. [模块化](#模块化)
12. [作用域](#作用域)
13. [内存安全](#内存安全)
14. [游戏开发入门](#游戏开发入门)

---

## 1. 快速开始

### 1.1 安装与运行

```bash
# 编译编译器
cd freq
make

# JIT 模式运行（直接执行，不生成可执行文件）
./freq -r test/example.fq

# 编译模式（生成可执行文件）
./freq test/example.fq -o output
./output
```

### 1.2 第一个程序

```freq
func main() {
    printn("Hello, freq!")
}
```

运行结果：
```
Running test/example.fq with JIT...
Hello, freq!

  Run: 2.5 ms
  Parse: 0.1 ms
  Compile: 0.5 ms
  Total: 3.1 ms
```

---

## 2. 变量与数据类型

### 2.1 变量声明

freq 支持三种变量声明方式：

```freq
var x = 10          // 普通变量，可修改
let result = get_value()  // 一次性常量，仅声明时可赋值
const PI = 3.14      // 编译时常量
```

### 2.2 数据类型

| 类型 | 关键字 | 说明 | 示例 |
|------|--------|------|------|
| 整数 | `int` | 32位有符号整数 | `var int age = 25` |
| 长整数 | `int64` | 64位有符号整数 | `var int64 big = 10000000000` |
| 单精度浮点 | `float` | 32位浮点 | `var float pi = 3.14f` |
| 双精度浮点 | `double` | 64位浮点 | `var double precise = 3.1415926` |
| 布尔 | `bool` | 布尔值 | `var bool flag = true` |
| 字符串 | `string` | 字符串 | `var string name = "freq"` |
| 字符 | `char` | 单个字符 | `var char c = 'A'` |

### 2.3 类型注解

```freq
// 显式类型声明
var int x = 10
var string name = "Hello"

// 自动类型推断
var y = 20      // int
var z = 3.14    // double
```

---

## 3. 运算符

### 3.1 算术运算符

```freq
var a = 10
var b = 3

printn(a + b)  // 13
printn(a - b)  // 7
printn(a * b)  // 30
printn(a / b)  // 3
```

### 3.2 比较运算符

```freq
var x = 5

printn(x == 5)  // true
printn(x != 5)  // false
printn(x > 3)   // true
printn(x < 3)   // false
printn(x >= 5)  // true
printn(x <= 5)  // true
```

### 3.3 自增/自减

```freq
var i = 0
i++  // i = 1
i--  // i = 0
```

---

## 4. 控制流程

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

**多值匹配：**
```freq
switch (num) {
    1, 3, 5: printn("Odd")
    2, 4, 6: printn("Even")
}
```

**范围匹配：**
```freq
switch (age) {
    0..17: printn("Minor")
    18..64: printn("Adult")
    65..120: printn("Senior")
}
```

### 4.3 while 循环

```freq
var i = 0
while (i < 5) {
    printn(i)
    i++
}
```

**带退出条件的 while：**
```freq
var attempts = 0
while (true) {
    printn("Attempt: ", attempts)
    attempts++
} exit(attempts >= 3)
```

### 4.4 for 循环

```freq
for(var int i = 0; i < 5; i++) {
    printn(i)
}
```

### 4.5 fo 循环（For-Each）

**数组遍历：**
```freq
var arr = [1, 2, 3, 4, 5]
fo (arr) [val] {
    printn(val)
}
```

**范围遍历：**
```freq
fo (1 .. 5) [i] {
    printn(i)
}
```

### 4.6 循环控制

```freq
// break - 跳出循环
for(var int i = 0; i < 10; i++) {
    if (i == 5) break
    printn(i)
}

// continue - 跳过当前迭代
for(var int i = 0; i < 5; i++) {
    if (i == 2) continue
    printn(i)
}

// quit / exit - 退出循环，继续执行后续代码
while (true) {
    printn("First")
    quit
}
printn("After loop")
```

---

## 5. 函数

### 5.1 函数定义

**风格一：类型在前（C 风格）**
```freq
func add(int a, int b) {
    return a + b : int
}
```

**风格二：类型在后（Go 风格）**
```freq
func add(a:int, b:int) {
    return a + b : int
}
```

### 5.2 函数调用

```freq
func main() {
    var result = add(1, 2)
    printn(result)  // 3
}
```

### 5.3 返回值

```freq
func get_char() {
    return 'A' : char
}

func divide(x:float, y:float) {
    return x / y : float
}
```

### 5.4 主函数

```freq
func main() {
    // 程序入口
}
```

---

## 6. 数组

### 6.1 一维数组

```freq
var arr = [1, 2, 3, 4, 5]
printn(arr[0])   // 1
printn(arr[2])   // 3
```

### 6.2 字符数组

```freq
var chars = ['a', 'b', 'c']
printn(chars[1])  // b
```

### 6.3 多维数组

```freq
var matrix = [[1, 2], [3, 4], [5, 6]]
printn(matrix[0][0])  // 1
printn(matrix[1][1])  // 4
```

### 6.4 数组属性

```freq
var arr = [1, 2, 3, 4, 5]
var size = arr.len  // 5

for(var int i = 0; i < arr.len; i++) {
    printn(arr[i])
}
```

---

## 7. 结构体

### 7.1 定义结构体

```freq
struct Point {
    x: int
    y: int
}
```

### 7.2 创建实例

```freq
var p = Point {
    x: 10
    y: 20
}
```

### 7.3 访问成员

```freq
printf("x = %d\n", p.x)
printf("y = %d\n", p.y)
```

### 7.4 修改成员

```freq
p.x = 100
p.y = 200
```

### 7.5 复杂结构体

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

## 8. 枚举

### 8.1 定义枚举

```freq
enum Color {
    RED,
    GREEN,
    BLUE
}

enum BaudRate {
    B9600,
    B19200,
    B115200 = 115200  // 手动赋值
}
```

### 8.2 使用枚举

```freq
func main() {
    var c = Color.RED      // 0
    var baud = BaudRate.B115200  // 115200
    printn(Color.GREEN)   // 1
}
```

---

## 9. 指针

### 9.1 指针声明

```freq
var ptr:*int  // 指向 int 的指针
```

### 9.2 取地址

```freq
var x:int = 5
var ptr:*int = &x  // 获取 x 的地址
```

### 9.3 解引用

```freq
var val = *ptr  // 获取指针指向的值
printn(val)     // 5

*ptr = 10       // 修改指针指向的值
printn(x)       // 10
```

---

## 10. 类型转换

### 10.1 C 风格转换

```freq
var int x = (int)3.14
var float y = (float)42
```

### 10.2 方法风格转换

```freq
var int x = 3.14.int()
var float y = 42.float()
```

**支持的转换函数：**
- `int()` → 整数
- `int64()` → 长整数
- `float()` → 单精度浮点
- `double()` → 双精度浮点
- `char()` → 字符
- `bool()` → 布尔
- `string()` → 字符串

---

## 11. 模块化

### 11.1 创建模块

**math.fq：**
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

### 11.2 导入模块

**方式一：导入整个模块**
```freq
import math

func main() {
    var sum = math::add(1, 2)
    var product = math::multiply(3, 4)
    printn(sum)      // 3
    printn(product)  // 12
}
```

**方式二：导入特定函数（函数无需 out 关键字）**
```freq
use math::add

func main() {
    var sum = add(1, 2)
    printn(sum)  // 3
}
```

**方式三：导入导出变量（需要 out 关键字）**
```freq
use math::PI

func main() {
    printn(PI)  // 3.14
}
```

### 11.3 模块系统规则

| 关键字 | 作用 | 需要 out |
|--------|------|----------|
| `mod` | 声明模块 | - |
| `import` | 导入整个模块 | 否 |
| `use` | 导入特定成员 | 函数：否 / 变量：是 |
| `out` | 标记变量可导出 | - |
| `::` | 模块访问操作符 | - |

---

## 12. 作用域

### 12.1 全局变量

```freq
var global_var:int = 100

func main() {
    printn(global_var)  // 100
}
```

### 12.2 局部变量

```freq
func main() {
    var local_var:int = 50
    printn(local_var)  // 50
}
```

### 12.3 块级作用域

```freq
func main() {
    var a:int = 1
    if (a == 1) {
        var b:int = 2
        printn(b)  // 2
    }
    // printn(b)  // 错误：b 不可见
}
```

### 12.4 作用域查找规则

1. **最近优先**：先在当前作用域查找
2. **逐级向上**：找不到则向上级作用域查找
3. **全局兜底**：所有作用域都找不到则报错

---

## 13. 内存安全

### 13.1 未初始化变量检测

```freq
var x:int  // 未初始化
func main() {
    printn(x)  // Error: Variable 'x' is used without initialization
}
```

### 13.2 空指针检测

```freq
var ptr:*int
func main() {
    *ptr = 10  // Error: Dereferencing null pointer
}
```

### 13.3 正确用法

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

## 14. 游戏开发入门

### 14.1 游戏循环

```freq
struct GameState {
    score: int
    lives: int
    frame: int
}

func update_input(state:*GameState) {
    // 处理输入
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

### 14.2 精灵管理

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

### 14.3 碰撞检测

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

## 附录：输出函数

| 函数 | 描述 | 示例 |
|------|------|------|
| `print()` | 输出单个值 | `print("Hello")` |
| `printn()` | 输出单个值并换行 | `printn("World")` |
| `printf()` | 格式化输出 | `printf("Age: %d\n", age)` |
| `printfn()` | 格式化输出并换行 | `printfn("Name:", name)` |

**printf 格式说明符：**
- `%d` - 整数
- `%s` - 字符串
- `%c` - 字符
- `%p` - 指针地址
- `%f` - 浮点数

---

## 附录：转义字符

| 转义序列 | 描述 |
|----------|------|
| `\n` | 换行符 |
| `\t` | 制表符 |
| `\r` | 回车符 |
| `\\` | 反斜杠 |
| `\"` | 双引号 |

---

*freq Programming Language - Pure Vibe Coding for GUI, Games & Embedded Systems*

**Version:** 0.1.0
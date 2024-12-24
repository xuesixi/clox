
# clox
clox is a interpreter of the lox programming language. 

It has a compiler to compile scripts into bytecode, and a stack-based virtual machine to run_vm the bytecode.

## build

It currently requires GNU/readline.

* `$ make clox`: produce the executable "clox". You can also do `$ cc *.c -o clox -l readline`
* `$ make run_vm`: build, run_vm REPL, and then clean.
* `$ make file`: build, run_vm the file "test.lox", and then clean.

## usage
* `$ clox`: run_vm REPL
* `$ clox path/to/script`: run_vm a lox script 

Other options
* `-s`: show the compiled bytecode 
* `-d`: trace the execution process
* `-c path/to/output`: compile the script to bytecode and write to the specified output path.
    * e.g. `$ clox -c hello.byte hello.lox`

* `-b`: treat the provided file as bytecode
    * e.g. `$ clox -b hello.byte`


# the lox programming language

## style

* Most statements are terminated by a `;`
* As in c/java/javascript, structural statements do not need `;`
* `//`  a single line comment
* `/* comment */` is also supported

## type
Lox has the following built-in types
* `int` 
* `float`
* `bool`
* `nil`
* `string`
* `function`

## arithmetic

float and int support arithmetic operations.
* `+`: addition
* `-`: subtraction
* `*`: multiplication
* `/`: division. As in c, if both operands are int, it is a integer division.
* `%`: modulo. Can only be used between two int
* `**`: power. The result is always a float

## variable
* `var` to declare a variable. Lox is dynamic typed. A variable can hold values of different types. If not initialized, it has the default value of `nil`
* `const` to declare a constant variable which must be initialized and cannot be mutated.

```LOX
var name = "anda";
name = 10;
```

## scope
`{}` creates new scope. Variables in a inner scope can shadow variables in out scopes with the same name. No varibales can have the same name in one scope. 

The global scope is different. If a variable cannot be resolved at compiled time, it is assumed to be global. If such a variable does not exist at runtime, a runtime error occurs. 

Global variables are allowed to have the same name. The latest declaration will override old ones.

## `print`
Lox provides a built-in keyword `print` to output to stdout. A new line will be appended to the end automatically..

## condition and logical operators
* Only `nil` and `false` are considered "falsy". All other values, including empty string `""` and `0`, are true.
* `and`: return the first false value. If no false operands, return the last true value.
* `or`: return the first true value. If no true operands, return the last false value.

## control flow
Lox support `if/else/while/for` as in c/java/javascript

## function
Use the keyword `fun` to declare a function. Functions are first-class values in lox.
If a function does not explicitly return anything, it returns `nil`

```lox
fun greet(name) {
    print "good day: " + name;
}
greet("huhu");
```

## native functions

Clox has some built-in functions written in C.

* `clock()`: return the time (in seconds) since the program starts
* `int(input)`: convert string or float to int
* `float(input)`: convert int or string to float
* `rand(low, high)`: return a random int in [low, high]. Both arguments need to be int. 

# REPL

**REPL stands for "Read, Evaluate, Print, Loop".** It is an environment allowing you to write and test codes quickly.

If you run_vm `$ clox` without providing the path to a script, you will be in the REPL mode.

* It has a basic support for multipl-line input. 

* You can omit the `;` in this mode if it is the end of a statement.

* the results of all expression statements like `1 + 2` or `sum(1, 2, 3)` will be printed out automatically in gray color.

* The output of real `print` statements will be in green color.

Use `ctrl+D` or `ctrl+C` to quit REPL.

# Virtual Machine

the clox virtual machine is stack-based, which means most of its instructions operate on the stack. 

One bytecode is one byte. 

* `constants` is an array storing constants (similar to immediate value) used in a program. It is filled in during the compile time.
* `globals`: is a map storing global variables
* `stack`: the "main" stack
* the format `OP, index/offset` means that the instruction will use the next byte as its extra operand.
* Similarly, `OP, index16/offset16` means that the instruction will use the next two bytes (which is 16 bits) as its extra operand.
* If no format is specified, the instruction consists of a single opcode.
* `push()`: add a value into the top of the stack
* `pop()`: remove the value at the top of the stack
* `peek()`: look at the value at the top of the stack (without removing it)

## instructions

#### OP_RETURN

#### OP_POP 
```
pop()
```
#### OP_CONSTANT, index: 
```
value = constants[index]
push(value)
```
#### OP_CONSTANT2, index16: 
```
value = constants[index16]
push(value)
```

#### OP_NEGATE 
```
A = pop() 
push(-A)
```
#### OP_ADD 
```
B = pop()
A = pop() 
push(A + B)
```
#### OP_SUBTRACT 
```
B = pop() 
A = pop()
push(A - B)
```
#### OP_MULTIPLY
```
B = pop()
A = pop() 
push(A * B)
```
#### OP_DIVIDE 
```
B = pop() 
A = pop() 
push(A / B)
```
#### OP_MOD 
```
B = pop()
A = pop() 
push(A mod B)
```
#### OP_NIL 
```
push(nil)
```
#### OP_TRUE 
```
push(true)
```
#### OP_FALSE 
```
push(false)
```
#### OP_NOT 
```
A = pop(); 
push(!A);
```
#### OP_LESS 
```
B = pop()
A = pop() 
push(A < B)
```
#### OP_GREATER 
```
B = pop()
A = pop() 
push(A > B)
```
#### OP_EQUAL 
```
B = pop()
A = pop() 
push(A == B)
```
#### OP_PRINT 
```
A = pop()
print(A)
```
#### OP_DEFINE_GLOBAL, index: 
```
name = constants[index]
value = pop()
globals.set(name, value)
```

#### OP_DEFINE_GLOBAL_CONST, index: 
```
name = constants[index]
value = pop()
globals.set(name, value)
global_constant.add(name)
```
#### OP_GET_GLOBAL, index 
```
name = constants[index]
value = globals.get(name)
push(value)
```
#### OP_SET_GLOBAL, index 
```
name = constants[index]
value = pop()
globals.set(name, value)
```
#### OP_GET_LOCAL, index 
```
value = stack[index]
push(value)
```
#### OP_SET_LOCAL, index 
```
value = peek()
stack[index] = value
```
#### OP_JUMP_IF_FALSE, offset16 
```
if peek() is false:
	pc += offset16
```
#### OP_JUMP_IF_TRUE, offset16 
```
if peek() is true:
	pc += offset16
```
#### OP_JUMP, offset16 
```
pc += offset16
```
#### OP_JUMP_BACK, offset16 
```
pc -= offset16
```




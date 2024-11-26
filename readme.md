
# clox
clox is a virtual machine of the lox programming language. 

It has a compiler to compile scripts into bytecode, and a stack-based virtual machine to run the bytecode.

## build

It currently requires GNU/readline.

* `$ make clox`: produce the executable "clox"
* `$ make run`: build, run REPL, and then clean.
* `$ make file`: build, run the file "test.lox", and then clean.

## usage
* `$ clox`: run REPL
* `$ clox file`: run a lox script 

Other options to implement:
* `$ clox -c script_path output_path`: compile a script and write the bytecode to the specified path
* `$ clox -b bytecode_path`: run a bytecode file
* `$ clox -d bytecode_path`: disassemble a bytecode file

# the lox programming language

## type
Lox supports the following built-in types
* int 
* float
* bool
* nil
* String

## variable
* `var` to declare a variable. Lox is dynamic typed. A variable can hold values of different types. If not initialized, it has the default value of `nil`
* `const` to declare a constant variable which must be initialized and cannot be mutated.

## scope
`{}` creates new scope. Variables in a inner scope can shadow variables in out scopes with the same name. No varibales can have the same name in one scope. 

The global scope is different. If a variable cannot be resolved at compiled time, it is assumed to be global. If such a variable does not exist at runtime,
a runtime error occurs.

Global variables are allowed to have the same name. The latest declaration will override old ones.

## `print`
Lox provides a built-in keyword `print` to output to stdout. A new line will be appended to the end automatically..

## condition and logical operators
* Only `nil` and `false` are considered "falsy". All other values, including empty string `""` and `0`, are true.
* `and`: return the first false value. If no false operands, return the last true value.
* `or`: return the first true value. If no true operands, return the last false value.

## `if/else/while/for`
* as in c/java/javascript

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





# clox
clox is a virtual machine of the lox programming language. 

It has a compiler to compile scripts into bytecode, and a virtual machine to run the bytecode.

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

## print
Lox provides a built-in keyword `print` to output to stdout. A new line will be appended to the end automatically..

## condition and logical operators
* Only `nil` and `false` are considered "falsy". All other values, including empty string `""` and `0`, are true.
* `and`: return the first false value. If no false operands, return the last true value.
* `or`: return the first true value. If no true operands, return the last false value.



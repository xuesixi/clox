
# clox
I read the book [Crafting Interpreters](https://craftinginterpreters.com) and make an interpreter for the lox programming language. It has a compiler to compile scripts into bytecode, and a stack-based virtual machine to run the bytecode.

 I include some features that are not in lox standard but I think interesting. 

Lox is a dynamic-typed, gc language, similar to javascript and python. 

## docker/build

The implementation uses `asprintf()`  a lot. It also requires GNU/readline. A docker image is provided.

* `$ docker pull xuesixi/clox` : download the image

***

* `$ docker run --rm -itv $(pwd):/clox xuesixi/clox` : run the image and mount current directory into the `/clox`. You should run this command in the root of the project. 
  
	Once inside the container:
	
	* `$ cd /clox`: go the `/clox`
	
	* `$ make`: produce the executable "clox".  `$ make opt` does the same thing but apply O3 optimization. 

## usage
* `$ clox`: run REPL
* `$ clox path/to/script`: run a lox script 

Other options
* `-s`: show the compiled bytecode before execution
* ***`-d`: trace the execution process***
* ***`-c path/to/output`: compile the script to bytecode and write to the specified output path.***
    * e.g. `$ clox -c hello.byte hello.lox`

* ***`-b`: treat the provided file as bytecode***
    * e.g. `$ clox -b hello.byte`

# The Lox Programming Language 
(***with some personal extensions***)

## basic

* Most statements are terminated by a `;`
* As in c/java/javascript, structural statements do not need `;`
*  `//` can be used to start a single line comment
*  ***`# ` can also be used to start a single line comment***
* ***`/* comment */` is also supported***

## type
Primitive types: 
* ***`Int` : 32 bits integer***
* `Float`: 64 bits float point number
* `Bool`: `true`/`false`
* `Nil`: `nil`

Reference types:

* `String`: immutable. String supports indexing. `str[i]` returns the character at `i` as a string. 
* `object`
* ***`Array`***
* ***`Map`***

## arithmetic

float and int support arithmetic operations.
* `+`: addition
* `-`: subtraction
* `*`: multiplication
* `/`: division. As in c, if both operands are int, it is a integer division.
* ***`%`: modulo. Can only be used between two int***
* ***`**`: power. The result is always a float***

***`+=, -=, *=, /=, %=`  are also supported.***

## variable
* `var` to declare a variable. Lox is dynamic typed. A variable can hold values of different types. If not initialized, it has the default value of `nil`
* ***`const` to declare a constant variable which must be initialized and cannot be reassigned.***

```LOX
var name = "anda";
name = 10;
```

* ***`var` and `const` allow definining multiple variables at the same time. e.g. `var a, b, c = 1, 2, 3;` The value on the right hand side of the equal sign must be an array. It is an error if the length of the array is less than the number of variables to definie.***

## scope

`{}` creates a new scope. Variables in the inner scope can shadow variables in outer scopes with the same name. No varibales can have the same name in one scope. 

The global scope is different. If a variable cannot be resolved at compiled time, it is assumed to be global. If such a variable does not exist at runtime, a runtime error occurs. 

## print
Lox provides a built-in keyword `print` to output to stdout. A new line will be appended to the end automatically..

## condition and logical operators
* Only `nil` and `false` are considered "falsy". All other values, including empty string `""` and `0`, are true.
* `and`: return the first false value. If no false operands, return the last true value.
* `or`: return the first true value. If no true operands, return the last false value.

## control flow
Lox support `if/else/while/for` as in c/java/javascript。***continue, break, switch, and for in are supported.***

`if` and `while` do not  require `()` around the condition.`if num == 10 {}` is valid.

For `for` statement, `()` is used to distinguish c style `for` loop and `for in` loop. 

### switch

You can use expressions (not just literal values) as cases. 

* Each case has only one statements.
* To have multiple statements in a case, group them into one block statement. 
* No `break` is needed. 

* `|` allows one case to match multiple values.

 `switch` is not faster than `if-else`. Cases are compared in order. 

```lox
var num = 10;
var a = 4;
switch num {
	case 1:
    	print 1;
    case a: { 
    	var money = 40;
    	print money; 
    } 
    case a + 2 | 5 * a | 6:
    	print "huhu";
    default:
    	print 998;
}
```

### for in
It requires the value after `in` to be iterable.

* Iterable: has the `iterator` property: `iterator(): Iterator` and map are iterable. 
* Iterator: an object with `has_next` and `next` callable properties. 
    * `has_next(): Bool`
    * `next(): any`

```lox
var arr = 1, 2, 3;
for i in arr {
	print i;
}

for i in range(10) { // range is a optimized function
	print i;
}

var map = {
	1: 10,
	2: 20,
	3: "huhuh",
};

for k, v in map {
	print k;
	print v;
}
```

## function

Use the keyword `fun` to declare a function. Functions are first-class values in lox. 

Functions can be nested, You can define functions inside functions. Closure is supported. 

If a function does not explicitly return anything, it returns `nil`

```lox
fun adder(num1) {
	fun add_num1(num2) {
		return num1 + num;
	}
	return add_num1;
}
```

### anonymous function

***The `fun` keyword can also be used to define anonymous functions by simply omitting the function name. `$` does the same thing.***

Anonymous functions are expressions rather than statements.

```lox
var hey = fun (name) {
	print name;
};

var hello = $(name) {
	print name;
};
```

### optional parameter

***When defining a function, we can give parameters default values. At runtime, if the coressponding argument is absent, the value will be computed by evaluating the expression.***

```lox
fun add(a, b = 2 * a + 1) {
	// if b is absent, b = 2 * a + 1;
	return a + b;
}
```

All optionals parameters must go after any definite parameters. 

### var arg

***Adding `...` to the last parameter allows passing variable arguments. It is treated as an array at runtime.***

```lox
fun hey(greeting = "good day", names...) {
	for name in names {
		print f("#, #", greeting, name);
	}
}
```

## class

use the `class` keyword to define a class. Classes are also first-class values. Class definitions are also be nested inside functions. 

```LOX
class Dog: Animal {

	static dog_count = 0;

    static show_count() {
        print f("the count is #", Dog.dog_count);
    }

	init(name, age) {
		this.name = name;
		this.age = age;
		Dog.dog_count += 1;
	}

	eat(food) {
		print f("the dog # is eating #", this.name, food);
	}
	
	run() {
		super.run();
		print f("the dog # is also running!", this.name);
	}
}

var dog = Dog("anda", 4);
dog.eat("moria"); 
Dog.show_count();

```

### method 

Inside a function, you can define methods (you don't need `fun` here). `this` is supported and bounded to an object. It should work as you may expect. 

### init

Like python, lox does not use the `new` keyword to create an instance. `init` is a special method that is called on the new created instance if defined. . 

### static

***Inside a function, you can use`static` to define static functions or variables. They belong to the class rather than a specific instance.***

### inheritance

`class Dog: Animal {}`  suggests that Animal is the superclass of Dog. 

A subclass method can use `super.` to access a method of the superclass (this is only needed when a method shadows a 

same-name method of its superclass)

## data structure

***Clox provides two built-in data structures: array and map.***

### array

***An array has a fixed length and cannot grow.***

* `[n]` creates an array of length n. `[x][y][z]` creates a 3-d array with dimensions: x, y, z respectively. 
* `,` can be used to create array literal, for example `x, y, z` creates an array of length 3, containing items x, y, and z
* Arrays have only one field: `length`, but have some other methods. 
* Arrays are iterable.

```lox
var two_d = [3][4];

two_d[0][1] = "hello world!";

var arr = 1, 3, 9, 21, 73;

print arr.length; // 5

for i in arr {
	print i;
}
```

### map

***A map stores key-value pairs.***

* A map has a`length` field.
* In context where an expression is expected, `{}` creates a map. We can initialize the map with `key : value` pairs separated by `,`.
* If the key does not exist in the map, `map[key]` returns `nil`.
* Map is also iterable. The iterator returns the key-value pair array (of length 2) each time. Map is unordered. 

```lox
var m = {
	"name": "anda",
	1: 10,
	2: 99,
	true: 43,
	nil: Dog("mohu", 4)
};

var name = m["name"]; // map get
m[true] += 3; // map set

for k, v in m {
	print f("key: #, value: #", k, v);
}
```

Only values have callable properties `hash` and `equal` can be used as keys in maps.

* `hash(): int`
* `equal(another_value): bool`

 The map stores hash values of keys internally and expect they never change. 

The builtin equality test functions for `int, float, bool, nil, string` are based on "value." Other builtin types like function, module, class, array, and map are based on reference. 

If you want a user-defined class to be used as the key, you have to implement its `hash` and `equal` manually. A user-defined class **does not** have default `hash` and `equal` implementations.

## module

***A file is considered a module.***

### export

***You can use export before `var`, `const`, `fun`, `class`, if so, the defined values are exported. You can also use `export `statement to export multiple values at a time.*** 

```lox
export var num = 10;

fun hey(name) {
	print "good day: " + name;
}
class Animal {}
var time = 998;

export hey, Animal, time; // export multiple members at a time
```

### import

***`import` loads another module. Only members exported by `export` can be imported into another module.***

* Import a whole module`import "path/to/animal.lox" as animal;`. If you import the whole module, it creates a module variable with the name specified by `as`. You can then access it from the module name, e.g. `animal.Dog`	
* Import specific members. `import "path/to/animal.lox":Dog, Cat;` In such case, you can use `Dog`, `Cat` directly.
* Rename. `import "path/to/animal.lox": Dog as ModuleDog, Cat;` You can use `as` to rename one or more members to prevent naming conflicts. 
* The path is relative to the directory of current module (at compile time). 
## type annotation

***You can have type annotation for function parameter or variable. Type annotation is basically embeded comment, which is completely ignored by the interpreter.*** 

`|` may be used to represent union of types, like `Int|Float`

```lox
var name: String = "anda";
const num: Int = 10;

fun test(num: Int|Float, name: String): Bool {}
```

## Error Handling

***Similar to many other languages, you may use `throw` to throw a value and `try-catch` block to handle errors.*** 

All values can be thrown, but we typically throw values of the `Error` class (or its subclasses).

A try block must be followed by one or more catch clauses. 

Each catch clause may specifies the type of values it wants to catch. A catch clause may catch values of multiple types by using `|`.

```lox
try {
	var arr = [3];
	print arr[5]; // This will throw an IndexError!
} catch str: String {
	print "catch String!";
} catch err: IndexError { // this clause will catch the thrown value
	print "catch IndexError!";
} catch err: Error {
	print err.message;
	print err.position;
} catch err { // if no type is specified, it becomes an unconditional catch
	print "this is an unconditional catch!";
}

try {
	throw 998;
} catch num: Int|Float {
	print "catch Int/Float!";
} 
```

Values that are instances of `Error` (or its subclass) have `message`  (error description) and `position` (backtrace) properties. 

Common error types include:

* `TypeError`: perform operations on values of inappropriate types
* `ValueError`: "bad input" (but typically with correct type)
* `ArgError`: incorrect amount of arguments to functions
* `IndexError`: index out of bound for arrays, or use non-existent keys for maps
* `NameError`: access undefined variables
* `PropertyError`: access undefined properties
* `FatalError`: serious errors like stack overflow
* `IOError`: cannot find the file to import
* `CompileError`: the module to import fails to compile. 

## native functions

Clox has some built-in functions written in C.

* `clock(): Float`: return the time (in seconds) since the program starts
* `int(input: String|Float): Int`: convert string or float to int
* `float(input: String|Int): Float`: convert int or string to float
* `rand(low: Int, high: Int): Int`: return a random int in [low, high]. Both arguments need to be int. 
* `f(format: String, values...): String`: return a formated string according to the specify format. `#` is used as the placeholder. 
    * For example, ` var str = f("the name is #, age is #", "anda", 22)`
* `read(prompt: String): String`: read a line of string from the keyboard (excluding the newline). If `prompt` is provided, it will be printed out first.
* `type(value): Class`: return the type (a class object) of the input
* `backtrace(): String`: return a string representing the call frames.
* `value_of(value, t: Class): Bool`: return if the give value is of the given type.
* `is_object(value): Bool`: return if the given value is an "object" (not Int, Float, Bool, Nil, String, Array, Map, Function, Module, Class...)
* Some other functions with names starting with `native_`. The standard library provides some wrapper functions over them. For example, `range` is a wrapper over `native_range` that supports optional parameters and is more convenient to use. 

# REPL

**REPL stands for "Read, Evaluate, Print, Loop".** It is an environment allowing you to write and test codes quickly.

If you run `$ clox` without providing the path to a script, you will be in the REPL mode.

* It has a basic support for multipl-line input. 

* You can omit the `;` in this mode if it is the end of a statement.

* the results of all expression statements like `1 + 2` or `sum(1, 2, 3)` will be printed out automatically in gray color.

* The output of real `print` statements will be in green color.

Use `ctrl+D` , `ctrl+C` , or `exit()`to quit REPL.

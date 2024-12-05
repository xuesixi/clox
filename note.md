
# 总结

编译时，我们创造出函数的“原型”，也就是LoxFunction对象。

当运行到函数定义语句的时候，我们生成新的Closure对象，用其包装LoxFunction，并在此时由外层函数将closure需要的
upvalue转交给closure。closure通过upvalues来访问那个upvalue。
当一个upvalue对应的值离开栈时，我们对它进行“close”，也就是将其值独立地保存起来。

## 函数的编译

## scope
scope是用来辅助编译的一种数据结构，在运行时并不存在。在运行时，一个scope会对应一个CallFrame
```c
typedef struct Scope {
    Local locals[UINT8_MAX + 1];
    UpValue upvalues[UINT8_MAX + 1];
    LoxFunction *function;
    struct Scope *enclosing;
    FunctionType functionType;
    int local_count;
    int depth;
} Scope;

typedef struct UpValue {
    int index; // 这个upvalue在下一外层的upvalues或者locals中的索引
    bool is_local; // 这个upvalue对于下一外层的scope来说，是local还是另一个upvalue
} UpValue;
```
当我们开始编译，或者遇到一个函数的时候，我们会创建一个新的scope，并让原本的scope作为新的scope的enclosing。
最外层的全局scope的enclosing为NULL。

**值得注意的是，block语句并不会创建新的scope，而是仅仅增加depth。** 
depth在同一个函数内部区分不同的层级。

当我们申明本地变量的时候，在scope中添加一个local。

访问一个变量的时候，有三种可能，local, upvalue, global

### local 
尝试`resolve_local()`，也就是从当前scope的locals中寻找。

如果找到了，获得一个offset，这个index代表相对于帧指针（FP）的偏移值。然后生成 op-get/set-local, offset指令

运行时，依靠这个偏移值offset来访问对应的本地变量

### upvalue
如果local中没有找到，尝试`resolve_upvalue()`。

这个函数是这样的：
```c
/**
 * 已知给定的identifier不存在于scope中，将其视为upvalue，向外层寻找
 * @return 如果找到了，将之添加到scope的upvalues中，然后返回其索引。如果没找到，返回-1
 */
static int resolve_upvalue(Scope *scope, Token *identifier);
```
存在三种可能：
* 寻找的变量直接存在于外层的locals中的i位置，那么我们在当前scope中新添一个 `{index: i, is_local: true}`的upvalue对象
* 如果存在于外层的upvalues中的i位置，那么我们在当前scope中新添一个 `{index: i, is_local: false}`的upvalue对象
* 如果一直向外找都没有找到，返回-1.

前两个情况会返回新增的那个upvalue对象在当前scope中的upvalues数组中的索引。

如果resolve_upvalue找到了那个变量，那么我们生成 op-get/set-upvalue, index指令

### global

如果也没找到，那么视为global。我们将这个变量的名字记录在constant中。在运行时再去查找。

***
当一个函数编译完成后，我们将这个函数对象储存入constants中，获取其index，然后生成 op-closure, index指令。

此外，该函数每有一个upvalue，就生成一对 is_local, index 字节码

## 运行时的函数

函数的绝大部分内容 （主要是chunk）都是在编译时就被写好了，储存在某个constants中。
当我们运行诸如 `fun hey() { ... }`的函数定义（也就是op-closure指令）时，实际上只是从constants中找到那个函数，将其包装成closure，
然后将其置入栈中，或者存入globals中。

虽然编译函数时，是先编译内层函数。但运行op-closure时，是先运行外层的函数
（之后要等到外层函数被调用时，才会轮到内层函数的定义）。

运行时，我们看到op-closure, index指令，我们从constants中找到那个函数，然后将其包装成一个Closure对象。

```c
typedef struct LoxFunction {
    Object object;
    int arity;
    Chunk chunk;
    String *name;
    int upvalue_count;
} LoxFunction;

typedef struct Closure {
    Object object;
    LoxFunction *function;
    UpValueObject **upvalues;
    int upvalue_count;
} Closure;

typedef struct UpValueObject {
    Object object;
    Value *position;
    Value closed;
    struct UpValueObject *next;
} UpValueObject;
```

op-closure指令后面可能会有 is_local, index 字节码，我们读取这些字节码，
然后填充closure内部的`UpValueObject **upvalues;`属性。
这一过程是这样的：
```c
for (int i = 0; i < closure->upvalue_count; ++i) {
    bool is_local = read_byte();
    int index = read_byte();
    if (is_local) {
        closure->upvalues[i] = capture_upvalue(curr_frame()->FP + index);
    } else {
        closure->upvalues[i] = curr_frame()->closure->upvalues[index];
    }
}
```
在执行op-closure的指令时，当前的frame是那个函数的外层的frame。
因此，如果 is_local为真，那个upvalue所捕获的，正是当前frame中的对应index上的本地变量。
如果为假，它捕获的就是upvalue，也就是当前frame的closure的upvalue。

请注意，编译时的捕获是内层向外层寻找。运行时，则是当外层函数在执行内层函数的定义时，将值交给内层。


```c
/**
 * 返回一个捕获了当前处于value位置上的那个值的UpValueObject对象。
 * 如果那个只是第一次被捕获，新建UpValueObject，将其添加入链表中，然后返回之，否则沿用旧有的。
 */
static UpValueObject *capture_upvalue(Value *value);
```
在vm中，存在一个叫做 `open_upvalues`的有序链表。它保存所有仍存在于stack中的UpValueObject。
当我们用capture_upvalue来捕获一个值的时候，我们会先查看那个stack位置是否已经被捕获了。这可以让多个兄弟closure使用同一个UpValueObject。
如果还没被捕获，则创建一个具有对应的值的UpValueObject对象。

op-get/set-upvalue的执行很简单，在closure的upvalues中找到直接的对象，通过position字段访问/修改其值即可，

position是一个指针。如果捕获的栈上的值在其他地方被修改，position可以反映这种变化。

***

目前的closure仅仅捕获了栈上的值，当被捕获的值离开栈后，UpValueObject就会保存失效的值。为了让closure真正有效，我们需要额外的工作。

```c
/**
 * 将vm.open_upvalues中所有位置处于position以及其上的upvalue的值进行“close”，
 * 也就是说，将对应的栈上的值保存到UpValueObject其自身中。
 */
static void close_upvalue(Value *position) {
    UpValueObject *curr = vm.open_upvalues;
    while (curr != NULL && curr->position >= position) {
        curr->closed = *curr->position; // 把栈上的值保存入closed中
        curr->position = &curr->closed; // 重新设置position。如此一来upvalue的get，set指令可以正常运行
        curr = curr->next;
    }
    vm.open_upvalues = curr;
}
```


在编译时，每一个local变量，具有额外的 `is_captured` 字段。当它被捕获的时候，该字段被设为true。

对于block语句来说，当一个本地变量被清理后，如果该变量被捕获，那么我们生成 op-close-upvalue，而非单纯的op-pop。
op-close-upvalue指令会调用 `close_upvalue(stack_top - 1)`。

当一个函数返回时，我们则运行`close_value(FP)`，保存这个函数内部的所有本地变量。




CFLAGS = -Wall -Wextra -Wformat -g 
SRC = chunk.c compiler.c debug.c io.c main.c memory.c object.c scanner.c table.c value.c vm.c native.c
OBJ = $(SRC:.c=.o)
TARGET = clox

all: $(TARGET)
	@make $(TARGET)
	@./clox -c liblox_iter liblox/Iter.lox
	@xxd -i liblox_iter lib_iter.h
	@make clean
	@make $(TARGET)
	@rm liblox_iter

$(TARGET): $(OBJ) 
	@$(CC) $(OBJ) -o $(TARGET) -l readline -lm

o: $(SRC)
	@$(CC) $(SRC) -o $(TARGET) -O3 -l readline -lm

%.o: %.c *.h
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: lldb
lldb: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline -lm 
	@lldb $(TARGET)

.PHONY: gdb
gdb: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline -lm
	@gdb $(TARGET)

.PHONY: clion_debug
clion_debug: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline -lm

.PHONY: clean
clean:
	@rm -f *.o

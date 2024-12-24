CFLAGS = -Wall -Wextra -Wformat -g 
SRC = chunk.c compiler.c debug.c io.c main.c memory.c object.c scanner.c table.c value.c vm.c native.c
OBJ = $(SRC:.c=.o)
TARGET = clox

all: $(TARGET)

$(TARGET): $(OBJ) 
	@$(CC) $(OBJ) -o $(TARGET) -l readline -lm

o: $(SRC)
	@$(CC) $(SRC) -o $(TARGET) -O3 -l readline -lm

.PHONY: run
run: $(TARGET)
	@./$(TARGET) -ds
	@rm $(OBJ)
	@rm $(TARGET)

f: file

%.o: %.c common.h %.h
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: lldb
lldb: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline -lm 
	@lldb $(TARGET)

.PHONY: gdb
gdb: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline -lm
	@gdb $(TARGET)

clion_debug: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline -lm

.PHONY: clean
clean:
	@rm -f *.o

.PHONY: file
file: $(TARGET)
	@./$(TARGET) -ds test.lox
	@rm $(OBJ)
	@rm $(TARGET)

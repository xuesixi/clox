CC = gcc
CFLAGS = -Wall -Wextra -Wformat -g
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = clox

$(TARGET): $(OBJ)
	@$(CC) $(OBJ) -o $(TARGET) -l readline
	@rm $(OBJ)

.PHONY: run
run: $(TARGET)
	@./$(TARGET) -ds
	@rm $(TARGET)


f: file

.PHONY: file
file: $(TARGET)
	@./$(TARGET) -ds test.lox
	@rm $(TARGET)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

debug: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline
	@lldb $(TARGET)
	@trash-put *.dSYM
	@trash-put $(OBJ)

clion_debug: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline

.PHONY: clean
clean:
	@rm $(OBJ)
	@rm $(TARGET)

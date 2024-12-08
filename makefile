CC = gcc
CFLAGS = -Wall -Wextra -Wformat -g
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = clox

$(TARGET): $(OBJ)
	@$(CC) $(OBJ) -o $(TARGET) -l readline

.PHONY: run
run: $(TARGET)
	@./$(TARGET) -ds
	@rm $(OBJ)
	@rm $(TARGET)


f: file

.PHONY: file
file: $(TARGET)
	@./$(TARGET) -ds test.lox
	@rm $(OBJ)
	@rm $(TARGET)

s: silence

.PHONY: silence
silence: $(TARGET)
	@./$(TARGET) test.lox
	@rm $(OBJ)
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

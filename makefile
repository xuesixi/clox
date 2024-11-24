CC = gcc
CFLAGS = -Wall -Wextra -Wformat -g
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = main.out

all: $(TARGET)

.PHONY: run
run: $(TARGET)
	@./$(TARGET)
	@rm $(TARGET)
	@rm *.o

.PHONY: file
file: $(TARGET)
	@./$(TARGET) test.lox
	@rm $(TARGET)
	@rm *.o

$(TARGET): $(OBJ)
	@$(CC) $(OBJ) -o $(TARGET) -l readline

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

debug: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline
	@lldb $(TARGET)
	@trash-put *.dSYM

clion_debug: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET) -l readline

.PHONY: clean
clean:
	@rm $(OBJ)

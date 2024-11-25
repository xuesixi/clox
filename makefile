CC = gcc
CFLAGS = -Wall -Wextra -Wformat -g
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = main.out

all: run

$(TARGET): $(OBJ)
	@$(CC) $(OBJ) -o $(TARGET) -l readline

.PHONY: run
run: $(TARGET)
	@./$(TARGET)
	@rm $(TARGET)
	@rm *.o


f: file

.PHONY: file
file: $(TARGET)
	@./$(TARGET) test.lox
	@rm $(TARGET)
	@rm *.o

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

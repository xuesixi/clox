CC = gcc
CFLAGS = -Wall -Wextra -Wformat -g
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = main.out

all: run

.PHONY: run
run: $(TARGET)
	@./$(TARGET)
	@rm $(TARGET)

$(TARGET): $(OBJ)
	@$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

debug: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET)
	@lldb $(TARGET)
	@trash-put *.dSYM

clion_debug: $(OBJ)
	@$(CC) -g $(OBJ) -o $(TARGET)

.PHONY: clean
clean:
	@rm $(OBJ)

CFLAGS = -Wall -Wextra -g 
SRC = chunk.c compiler.c debug.c io.c main.c memory.c object.c scanner.c table.c value.c vm.c native.c
OBJ = $(SRC:.c=.o)
TARGET = clox
LIB_HEADERS = liblox_iter.h liblox_core.h liblox_data_structure.h
C_HEADERS = chunk.h common.h compiler.h debug.h io.h memory.h native.h object.h scanner.h table.h value.h vm.h

all: $(TARGET) $(LIB_HEADERS)
	@rm native.o # so that load_libraries() is recompiled with the updated loxlib
	@make $(TARGET)

$(TARGET): $(OBJ) 
	@$(CC) $(OBJ) -o $(TARGET) -l readline -lm

%.o: %.c $(C_HEADERS)
	@$(CC) $(CFLAGS) -c $< -o $@

liblox_%.h: liblox_% 
	@xxd -i $< $@

liblox_%: liblox/%.lox $(TARGET)
	@./clox -c $@ $<

opt: $(SRC) $(LIB_HEADERS)
	@$(CC) $(SRC) -o $(TARGET) -O3 -l readline -lm # for optimized clox

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
	@rm clox

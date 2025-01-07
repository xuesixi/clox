CFLAGS = -Wall -Wextra 
LINK_FLAGS = -l readline -l m
SRC = chunk.c compiler.c debug.c io.c main.c memory.c object.c scanner.c table.c value.c vm.c native.c
OBJ = $(SRC:.c=.o)
TARGET = clox
LIB_HEADERS = liblox_iter.h liblox_core.h liblox_data_structure.h
C_HEADERS = chunk.h common.h compiler.h debug.h io.h memory.h native.h object.h scanner.h table.h value.h vm.h


.PHONY: all
all: $(LIB_HEADERS)
	@rm native.o && $(MAKE) $(TARGET)

liblox_%.h: liblox_% 
	@xxd -i $< $@

liblox_%: liblox/%.lox $(TARGET)
	@./clox -c $@ $<

.PHONY: $(TARGET)
$(TARGET): $(OBJ) 
	@$(CC) $(OBJ) -o $(TARGET) $(LINK_FLAGS)

%.o: %.c $(C_HEADERS)
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: opt
opt: all
	@$(CC) $(SRC) -o $(TARGET) -O3 $(LINK_FLAGS) # for optimized clox

.PHONY: debug
debug: $(SRC)
	@$(CC) -g $(SRC) -o $(TARGET) $(LINK_FLAGS) $(CFLAGS) -g

.PHONY: clean
clean:
	@rm -f *.o
	@rm clox

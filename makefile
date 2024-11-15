all: run

.PHONY: run
run: main 
	@./main.out
	@rm main.out

main: main.c memory.c chunk.c debug.c value.c
	@gcc main.c chunk.c memory.c debug.c value.c -o main.out

debug: 
	@gcc -g main.c memory.c chunk.c debug.c value.c -o main.out
	@lldb main.out
	@trash-put *.dSYM


all: run

.PHONY: run
run: main 
	@./main.out
	@rm main.out

main:
	@gcc *.c -o main.out

debug: 
	@gcc -g *.c -o main.out
	@lldb main.out
	@trash-put *.dSYM

clion_debug:
	@gcc -g *.c -o main.out
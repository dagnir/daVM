CC=clang++
OPTS=-Wall -std=c++11

all:
	$(CC) $(OPTS) main.cpp vm.cpp -o main

clean:
	rm -rfv main

CC=g++
WARNINGS= -Wall -Wno-missing-braces -Wextra -Wno-missing-field-initializers \
	-Wcast-align -Wconversion -Wpointer-arith -Wunreachable-code -Winline \
	-Wstrict-aliasing=2

OPTS=-pedantic -std=c++11

all:
	$(CC) $(OPTS) $(WARNINGS) main.cpp vm.cpp -o main

clean:
	rm -rfv main

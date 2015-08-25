CC=clang++
WARNINGS= -Wall -Wno-missing-braces -Wextra -Wno-missing-field-initializers \
	-Wcast-align -Wconversion -Wpointer-arith -Wunreachable-code -Winline \
	-Wstrict-aliasing=2

OPTS=-pedantic -std=c++11 -O2

all:
	$(CC) $(OPTS) $(WARNINGS) main.cpp vm.cpp -o davm

clean:
	rm -rfv davm

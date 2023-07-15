build: 
	@../../smake/smake.exe
b: build
all: build


run:
	@make b && ../../bin/main.exe
r: run


clean:
	@rm ../../bin/main.exe
c: clean

test:
	@../../tests/all_tests.exe
t: test
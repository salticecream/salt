build: 
	@cd smake && make r && cd ..
b: build

all_build:
	@cd smake && make r && cd .. && make b

all_run:
	@make all_build && make r

run:
	@bin/main.exe
r: run


clean:
	@rm bin/main.exe
c: clean

test:
	@tests/all_tests.exe
t: test

br:
	@make build && make r
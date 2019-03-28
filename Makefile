.PHONY : all
all: build_tests

.PHONY: build_tests
build_tests:
	make -C ./tests

.PHONY: clean
clean: tests_clean

.PHONY: tests_clean
tests_clean:
	make -C tests clean

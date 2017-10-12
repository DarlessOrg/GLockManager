all: test

test:
	py.test -s tests
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html

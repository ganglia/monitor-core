pys = example.py
EXTRA_DIST = $(pys)

if BUILD_PYTHON

pycs = example.pyc

.py.pyc:
	$(PYTHON_BIN) -c "import py_compile; py_compile.compile( '$<' )"

EXTRA_DIST += $(pycs)

endif

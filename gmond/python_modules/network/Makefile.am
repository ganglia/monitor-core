if BUILD_PYTHON

pys = tcpconn.py
pycs = tcpconn.pyc

.py.pyc:
	$(PYTHON_BIN) -c "import py_compile; py_compile.compile( '$<' )"

EXTRA_DIST = $(pys) $(pycs)

endif

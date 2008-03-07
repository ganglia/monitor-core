if BUILD_PYTHON
pys := ./example/*.py ./disk/*.py ./network/*.py
pycs := $(patsubst %.py, %.pyc, $(pys))

$(pycs): %.pyc: %.py
	$(PYTHON_BIN) -c "import py_compile; py_compile.compile( '$<' )"

EXTRA_DIST = ./conf.d/*.pyconf \
	$(pys) \
	$(pycs)
endif

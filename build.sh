#!/bin/sh

(cd amio; swig -python -py3 core.i) && python3.7 setup.py build_ext --inplace

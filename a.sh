#!/bin/bash

chmod -x *.h *.cpp *.ui *.pro *.md
python ../Confucius-Mencius/py_tools/base/utf8_convertor.py . .h .cpp
../Confucius-Mencius/sh_tools/base/dos2unix.sh .

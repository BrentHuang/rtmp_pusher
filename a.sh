#!/bin/bash

python ../Confucius-Mencius/py_tools/base/utf8_convertor.py . .h .cpp
../Confucius-Mencius/sh_tools/base/dos2unix.sh .
dos2unix *.pro # qt creator pro文件不能带utf8-bom，用dos2unix去掉
../Confucius-Mencius/sh_tools/base/chmod.sh .

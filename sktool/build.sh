#!/bin/sh

qmake -project
qmake
make clean
make -j4

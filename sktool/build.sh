#!/bin/sh

make distclean
qmake -project "LIBS += ../libraries/libskt/libskt.a"
qmake
make -j4

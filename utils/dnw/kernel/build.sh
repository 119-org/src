#!/bin/sh

version=$(uname -r)
path=$(pwd)

make -C /lib/modules/$version/build/ M=$path modules
sudo insmod ./secbulk.ko

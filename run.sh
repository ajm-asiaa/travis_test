#!/bin/bash

echo "Test: Running run.sh script"

export PATH=/opt/Qt/5.3/gcc_64/bin:$PATH

cd /cartabuild/CARTAvis/carta/html5/common/skel

./generate.py source-all > /dev/null

cd /cartabuild/CARTAvis/

mkdir build

cd build

qmake NOSERVER=1 CARTA_BUILD_TYPE=dev ../carta -r

make -j 2


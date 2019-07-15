# !/bin/sh

CTS_HOME=$(pwd)
THIRD_PARTY="$CTS_HOME/third_party" 

if [ $# -ne 2 ]; then
    echo "Usage: ./compileAll.sh <lemon_dir> <num_threads>"
	exit
fi

LEMON_DIR=$1
NUM_THREADS=$2

echo " >>> Compiling TritonCTS"
echo " - CTS home: $CTS_HOME"
echo " - Lemon dir set to: $LEMON_DIR"

echo " - Updating submodules"
git submodule update --init --recursive

# Compile CTS helper first...
cd third_party/CtsHelper
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j $NUM_THREADS
cp lefdef2cts $THIRD_PARTY
cd $CTS_HOME

# Compile TritonCTS
make LEMON_HOME=$LEMON_DIR -j $NUM_THREADS

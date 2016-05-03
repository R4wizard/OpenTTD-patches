#!/usr/bin/env bash

export CORES=4
export CNFFLAGS="--without-libbfd"

#Run it all
export PATH="/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:/opt/local/bin:/opt/local/sbin:$HOME/.homebrew/Cellar"
export CC="/usr/bin/gcc"
export CXX="/usr/bin/g++"
export LDFLAGS="-stdlib=libstdc++ -lstdc++"

./configure "$CNFFLAGS"
make -j$CORES

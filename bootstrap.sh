#!/bin/bash
CURL=$(which curl)
PUSHD=$(which pushd)
POPD=$(which popd)

readline_version="6.1"
readline_tar="readline-${readline_version}.tar.gz"
readline_repos="ftp://ftp.cwru.edu/pub/bash/${readline_tar}"

# Build cmockery
cmockery_version="0.1.2"
cmockery_tar="cmockery-${cmockery_version}.tar.gz"
cmockery_url="http://cmockery.googlecode.com/files/${cmockery_tar}"

# Fix for OSX not finding malloc.h
if [[ `uname -s` == "Darwin" ]]; then
    export CFLAGS=-I/usr/include/malloc
fi

if [[ !(-d "./build") ]]; then
    mkdir ./build
fi

if [ -f "build/cmockery/lib/libcmockery.a" ]; then
    echo "libcmockery built"
else
  if [ -f "/usr/local/lib/libcmockery.a" ]; then
      mkdir -p `pwd`/build/cmockery/{lib,include}
      cp /usr/local/lib/libcmockery.a "$(pwd)/build/cmockery/lib"
      cp -r /usr/local/include/google "$(pwd)/build/cmockery/include"
  elif [ -f "/usr/local/lib/libcmockery.a" ]; then
      mkdir -p `pwd`/build/cmockery/{lib,include}
      cp /usr/lib/libcmockery.a "$(pwd)/build/cmockery/lib"
      cp -r /usr/include/google "$(pwd)/build/cmockery/include"
  else
      pushd build
      prefix=`pwd`/cmockery
      $CURL -o $cmockery_tar $cmockery_url
      tar -xzf cmockery-${cmockery_version}.tar.gz
      pushd cmockery-${cmockery_version}
      ./configure --prefix=$prefix && make && make install
      popd
      rm -rf cmockery-${cmockery_version}
      popd
  fi
fi

if [ -f "build/readline/lib/libreadline.a" ]; then
    echo "readline built"
else
  pushd build
  prefix=`pwd`/readline
  $CURL -o $readline_tar $readline_repos
  echo $readline_tar
  tar -xzf $readline_tar
  mv readline readline-${readline_version}
  pushd readline-${readline_version}
  ./configure --prefix=$prefix && make && myake install
  popd
  rm -rf readline-${readline_version}
  popd
fi

AUTOCONF_VERSION=2.65
echo "- Installing autoconf version $AUTOCONF_VERSION"
if [ ! "$(which autoconf | grep $AUTOCONF_VERSION)" ]; then
  wget http://ftp.gnu.org/gnu/autoconf/autoconf-$AUTOCONF_VERSION.tar.gz
  tar xvzf autoconf-$AUTOCONF_VERSION.tar.gz
  cd autoconf-$AUTOCONF_VERSION
  ./configure --prefix=/usr
  make
  sudo make install
  popd
  rm -rf autoconf-$AUTOCONF_VERSION.tar.gz autoconf-$AUTOCONF_VERSION
else
  echo "autoconf installed"
fi

# Run autoconf
echo "- Running autoconf"
autoconf
echo "- Configuring"
./configure
echo "- Making"
make

# cleanup
rm -rf build/*.tar.gz
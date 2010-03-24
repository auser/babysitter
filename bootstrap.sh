#!/bin/bash
CURL=$(which curl)

esc="\\"

black="${esc}[30m";   red="${esc}[31m";    green="${esc}[32m"
yellow="${esc}[33m"   blue="${esc}[34m";   purple="${esc}[35m"
cyan="${esc}[36m";    white="${esc}[37m"

blackb="${esc}[40m";   redb="${esc}[41m";    greenb="${esc}[42m"
yellowb="${esc}[43m"   blueb="${esc}[44m";   purpleb="${esc}[45m"
cyanb="${esc}[46m";    whiteb="${esc}[47m"

boldon="${esc}[1m";    boldoff="${esc}[22m"
italicson="${esc}[3m"; italicsoff="${esc}[23m"
ulon="${esc}[4m";      uloff="${esc}[24m"
invon="${esc}[7m";     invoff="${esc}[27m"

reset="${esc}[0m"

CURRENT_STR=""

function cecho () {
  message=${1:-$default_msg}    # Defaults to default message.
  color=${2:-$black}            # Defaults to black, if not specified.
  newline=${3:-yes}             # Newline?

  CURRENT_STR="$color$message$reset"
  if [ "$newline" == "yes" ]; then
    echo -e $CURRENT_STR
    CURRENT_STR=""
  fi
}  

function aligned_msg () {
  answer=${1:-"?"}    # Defaults to default message.
  color=${2:-$green}
  printf "%-35s$color%s$reset\n" $CURRENT_STR "$answer"
  CURRENT_STR=""
}
function not_found_msg () {
  aligned_msg "not found" $red
}
function found_msg () {
  aligned_msg "found" $green
}

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

cecho "libcmockery..." $blue no
if [ -f "build/cmockery/lib/libcmockery.a" ]; then
  found_msg
else
  not_found_msg
  cecho "Building libcmockery" $green
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

cecho "readline..." $blue no
if [ -f "build/readline/lib/libreadline.a" ]; then
  found_msg
else
  not_found_msg
  cecho "Downloading and building readline" $red
  pushd build
  prefix=`pwd`/readline
  $CURL -o $readline_tar $readline_repos
  echo $readline_tar
  tar -xzf $readline_tar
  mv readline readline-${readline_version}
  pushd readline-${readline_version}
  ./configure --prefix=$prefix && make && make install
  popd
  rm -rf readline-${readline_version}
  popd
fi

AUTOCONF_VERSION=2.65
cecho "autoconf..." $blue no
if [ -n "$(which autoconf | grep $AUTOCONF_VERSION)" ]; then
  not_found_msg
  cecho "Downloading and building autoconf $AUTOCONF_VERSION"
  wget http://ftp.gnu.org/gnu/autoconf/autoconf-$AUTOCONF_VERSION.tar.gz
  tar xvzf autoconf-$AUTOCONF_VERSION.tar.gz
  pushd autoconf-$AUTOCONF_VERSION
  ./configure --prefix=/usr
  make
  sudo make install
  popd
  rm -rf autoconf-$AUTOCONF_VERSION.tar.gz autoconf-$AUTOCONF_VERSION
else
  found_msg
fi

# Run autoconf
cecho "Running autoconf..." $green
autoconf
cecho "Configuring... " $green
./configure
if [ "$?" != "0" ]; then
  cecho "Error configuring..." $red
fi
cecho "Making... " $green no
make
if [ "$?" != "0" ]; then
  cecho "Error making..." $red
else
  cecho "success" $green
fi

# cleanup
rm -rf build/*.tar.gz
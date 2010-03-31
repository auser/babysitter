#!/bin/bash
CURL=$(which curl)

esc=""

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

# Build cmockery
cpputest_version="v2.1"
cpputest_tar="CppUTest${cmockery_version}.zip"
cpputest_dir="CppUTest${cmockery_version}"
cpputest_url="http://sourceforge.net/projects/cpputest/files/cpputest/${cpputest_version}/${cpputest_tar}/download"

# Fix for OSX not finding malloc.h
if [[ `uname -s` == "Darwin" ]]; then
    export CFLAGS=-I/usr/include/malloc
fi

if [[ !(-d "./build") ]]; then
    mkdir ./build
fi

cecho "CppUTest..." $blue no
if [ -f "build/${cpputest_dir}/lib/libcmockery.a" ]; then
  found_msg
else
  not_found_msg
  cecho "Building CppUTest" $green
  pushd build
  prefix=`pwd`/cpputest
  # echo "$CURL -o $cpputest_tar $cpputest_url"
  # $CURL -o $cpputest_tar $cpputest_url
  # echo $cpputest_tar
  # unzip ${cpputest_tar}
  pushd ${cpputest_dir}
  make
  popd
  rm -rf ${cpputest_tar}
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
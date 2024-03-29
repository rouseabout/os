#!/bin/sh
set -e

g(){
banner $1
(cd $1 && ./port.sh)
}

g gmp
g libffi
g libiconv
g mpfr
g mpc
g ncurses
g zlib

g bash
g binutils
g bwbasic
g coreutils
g curl
g dash
g dialog
g dmidecode
g ed
g fbdoom
g ffmpeg
g findutils
g framebuffer-tetris
g gcc
g git
g hexedit
g joe
g less
g links
g lua
g lynx
g make
g moe
g mutt
g nano
g nasm
g ncdu
g python2
g sc
g socat
g sox
g sqlite
g tinycc
g units
g vim
g vttest
g xz
g yasm

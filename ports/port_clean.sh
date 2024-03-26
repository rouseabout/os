#!/bin/sh
set -e

g(){
(cd $1 && rm -rf $1*)
}

g gmp
g mpc
g mpfr
g ncurses

g bash
g binutils
g coreutils
g curl
g dmidecode
g ed
g fbdoom
g ffmpeg
g findutils
g framebuffer-tetris
g gcc
g joe
g less
g libiconv
g links
g lynx
g make
g mutt
g nano
g nasm
g ncdu
g sox
g tinycc
g vim
g vttest
g yasm

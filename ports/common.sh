OSHOME="$PWD/../.."
export PATH="$OSHOME/toolchain-i686-pc-elf/bin:$PATH"
export SYSROOT="$OSHOME/sysroot"
export CACHE="$OSHOME/.cache"
apply_patches(){
if test -d ../patches && test ! -e .patched; then
    for x in ../patches/*; do patch -p1 < $x; done
    touch .patched
fi
}
prep(){
test -e $CACHE/$2 || curl --remote-name --output-dir $CACHE --location $1
test -d $3 || tar -xf $CACHE/$2
cd $3
apply_patches
}

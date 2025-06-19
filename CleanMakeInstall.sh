#clean
rm -r tmp bin
#make
make thinstream-server  NB_LIB_THINSTREAM=1 ls -l
NB_LIB_LZ4_SYSTEM=1
#install
mv bin/make/release/Linux/unknown/cc/thinstream-server ../sys-thinstream-res
mkdir -p lib
mkdir -p include
SCRIPT_DIR="$(dirname "$(realpath "$BASH_SOURCE")")"
OPENSSL_DIR="$SCRIPT_DIR/deps/openssl"
XERCES_DIR="$SCRIPT_DIR/deps/xerces-c"
cd $OPENSSL_DIR
./Configure mingw64 no-shared --cross-compile-prefix=x86_64-w64-mingw32-
make
cp libcrypto.a "$SCRIPT_DIR/lib"
rm -rf "$SCRIPT_DIR/include/openssl"
cp -rf include/openssl "$SCRIPT_DIR/include/openssl"
cd $XERCES_DIR
cmake . -DCMAKE_TOOLCHAIN_FILE=$SCRIPT_DIR/toolchain.cmake -Bbuild -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=build
cmake --build build
cmake --install build
cp build/lib/libxerces-c.a "$SCRIPT_DIR/lib"
rm -rf "$SCRIPT_DIR/include/xercesc"
cp -rf build/include "$SCRIPT_DIR"
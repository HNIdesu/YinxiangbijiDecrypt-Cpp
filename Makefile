build_debug:
	cmake . -Bbuild 
	cmake --build build

build_windows_debug:
	cmake . -Bbuild  -DCMAKE_TOOLCHAIN_FILE=./toolchain.cmake
	cmake --build build

clean:
	rm -r build
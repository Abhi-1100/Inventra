@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
cmake -B build_test -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64" -DCMAKE_BUILD_TYPE=Debug
cmake --build build_test

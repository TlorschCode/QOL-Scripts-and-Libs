@echo off

nasm -f win64 src/lib/asm_rand.asm -o asm_rand.obj
echo Compiled ASM

g++ -std=c++20 -O3 -march=native -Isrc/lib ^
    src/lib/asm_rand.cpp ^
    src/main.cpp ^
    asm_rand.obj ^
    -o main.exe

cmd /k main.exe

#! /usr/bin/bash
set -e
g++ -std=c++17 -fno-ident -fno-rtti -fno-exceptions -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -fmerge-all-constants main.cpp -o a -march=broadwell -O2 -flto -ffunction-sections -fdata-sections -s -Wl,-z,norelro -Wl,-z,max-page-size=0x1000 -Wl,--gc-sections -Wl,-z,noseparate-code
strip a -s -R .comment -R .gnu.version -R .note.gnu.property -R .note.gnu.build-id -R .note.ABI-tag -R .gnu.hash
tar -czf b a
ls -l
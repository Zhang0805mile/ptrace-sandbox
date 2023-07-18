#!/bin/sh
//这段脚本的作用是检查指定目录中的文件，并根据需要将文件复制到另一个目录中。
if [ ! -d src/ptrace-gen ]; then
    mkdir ./src/ptrace-gen
fi
if [ ! -d obj/ptrace-gen ]; then
    mkdir -p ./obj/ptrace-gen
fi
//for循环，用于遍历目录./src/ptrace-gen-scelet下的所有文件（不包括copy_myself.sh文件）。对于每个文件，首先获取其文件名（不包含路径）保存到file变量中。

for full_file in $(find ./src/ptrace-gen-scelet -type f | grep -v copy_myself.sh); do
    file=$(basename $full_file)
    if [ -f ./src/ptrace-gen/$file ]; then
        cmp -s $full_file ./src/ptrace-gen/$file 
        if [ $? -eq 1 ]; then
            cp $full_file ./src/ptrace-gen/$file
        fi
    else
        cp $full_file ./src/ptrace-gen/$file
    fi
done

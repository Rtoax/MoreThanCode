#!/bin/bash

# 删除 callgrind.out.12323 文件
rm -f $(ls | grep "[callgrind|cachegrind].out.[0-9]\+")

rm -f *.out

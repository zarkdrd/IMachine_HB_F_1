#########################################################################
# 
# File Name: cscope_ctags.sh
# Author: Mr Wei
# Descripttion:
# Created Time: 2021-07-12
#########################################################################
#!/bin/bash

find . -name "*.h" -o -name "*.c" -o -name "*.cpp" > cscope.files

cscope -Rbq -i cscope.files

ctags -R --languages=c,c++

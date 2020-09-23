#!/bin/bash

PROJECTHOME=$(pwd)
sysOS=`uname -s`
LLVMHome="llvm-10.0.0.obj"
install_path=/Users/yulei/Documents/workspace
export LLVM_DIR=$install_path/$LLVMHome
export PATH=$LLVM_DIR/bin:$PATH
export PATH=$PROJECTHOME/bin:$PATH
if [[ $sysOS == "Darwin" ]]
then 
    export SVF_DIR=$install_path/SVF/
elif [[ $sysOS == "Linux" ]]
then 
    export SVF_DIR=$install_path/SVF/
fi 

echo "LLVM_DIR="$LLVM_DIR
echo "SVF_DIR="$SVF_DIR

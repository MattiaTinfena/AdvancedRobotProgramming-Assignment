#!/bin/sh

sudo apt install terminator
sudo apt install konsole
sudo apt install libncurses-dev
sudo apt install libcjson-dev

make clean
make 

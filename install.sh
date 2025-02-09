#!/bin/sh

sudo apt-get update
sudo apt-get upgrade
sudo pt-get install --fix-missing

sudo apt install terminator
sudo apt install konsole
sudo apt install libncurses-dev
sudo apt install libcjson-dev

make clean
make 

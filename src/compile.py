#!/usr/bin/python

import os

os.system("killall Main.exe")
os.system("rm Main.exe")
os.system("killall Main_Child.exe")
os.system("rm Main_Child.exe")
os.system("g++ -std=c++0x Main.cpp -o Main.exe")
os.system("g++ -std=c++0x Main_Child.cpp -o Main_Child.exe")

os.system("./Main.exe")
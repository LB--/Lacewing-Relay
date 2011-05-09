@echo off

cl -o gen.exe gen.cc
gen
del *.obj
del *.exe


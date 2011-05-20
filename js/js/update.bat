@echo off

vcvarsall
cl -o gen.exe gen.cc
gen
del *.obj
del *.exe


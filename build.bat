@echo off
cd /D "%~dp0"
set PATH=C:\Psyq\bin;
set PSX_PATH=C:\Psyq\bin
set LIBRARY_PATH=C:\Psyq\lib
set C_PLUS_INCLUDE_PATH=C:\Psyq\include
set C_INCLUDE_PATH=C:\Psyq\include
set PSYQ_PATH=C:\Psyq\bin
set COMPILER_PATH=C:\Psyq\bin
set GO32=DPMISTACK 1000000 
set G032TMP=C:\WINDOWS\TEMP
set TMPDIR=C:\WINDOWS\TEMP
ccpsx -O3 -Xo$80040000 src/main.c src/player.c src/clip.c src/cd.c src/level.c src/col.c src/model.c src/sound.c src/entity.c src/util.c MMGMNEW.OBJ -otest.cpe,test.sym
cpe2x test.cpe >nul
del test.cpe

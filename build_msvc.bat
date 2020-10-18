@echo off
rem launch this from msvs-enabled console

set CXXFLAGS=/std:c++17 /O2 /FC /W4 /WX /nologo
set INCLUDES=/I SDL2\include
set LIBS=SDL2\lib\x64\SDL2.lib SDL2\lib\x64\SDL2main.lib Shell32.lib

cl.exe %CXXFLAGS% %INCLUDES% /Fesowon main.c /link %LIBS% -SUBSYSTEM:windows
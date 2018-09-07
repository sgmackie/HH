@echo off

:: Set build directory relative to current drive and path
set BuildDir=%~dp0..\..\build

:: Create path if it doesn't exist
if not exist %BuildDir% mkdir %BuildDir%

:: Move to build directory
pushd %BuildDir%

:: Set compiler arguments
set Files=..\handmade\code\win32_handmade.cpp
set Libs=user32.lib gdi32.lib
set ObjDir=.\obj\

:: Set compiler flags:
:: -DHANDMADE_WIN32 for performance metrics
:: -Zi enable debugging info
:: -FC use full path in diagnostics
:: -Fo path to store Object files
set CompilerFlags=-DHANDMADE_WIN32=1 -Zi -FC -Fo%ObjDir%

:: Create Object directory if it doesn't exist
if not exist %ObjDir% mkdir %ObjDir%

:: Run Visual Studio compiler
cl %CompilerFlags% %Files% %Libs%

:: Jump out of build directory
popd
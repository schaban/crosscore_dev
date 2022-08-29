@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

call tdm.bat

set PATH=%PATH%;%TDM_BIN%

set SRC_DIR=src
set INC_DIR=inc
set BIN_DIR=bin
set TMP_DIR=tmp
set OUT_DIR=%BIN_DIR%\gcc_%TDM_MACHINE%

set PROG_NAME=crosscore_demo

set DBG_FLG=-ggdb

set CPP_OFLGS=-O3 -flto
set EXE_MODE=Release
if x%1==xdebug (
	set PROG_NAME=!PROG_NAME!_dbg
	set CPP_OFLGS=-Og %DBG_FLG%
	set EXE_MODE=Debug
)
if x%1==xreldbg (
	set CPP_OFLGS=!CPP_OFLGS! %DBG_FLG%
	set EXE_MODE=Release with Debug
)

set PROG_EXE_PATH=%OUT_DIR%\%PROG_NAME%.exe

if not exist %OUT_DIR% mkdir %OUT_DIR%

set SRC_FILES=
for /f %%i in ('dir /b %SRC_DIR%\*.cpp') do (
	set SRC_FILES=!SRC_FILES! %SRC_DIR%/%%i
)


set XCORE_FLAGS=-DOGLSYS_ES=0 -DOGLSYS_CL=0 -DDRW_NO_VULKAN=1 -DXD_TSK_NATIVE=1
set CPP_OPTS=%CPP_OFLGS% -std=c++11 -mavx -mf16c -mfpmath=sse -ffast-math -ftree-vectorize -Wno-psabi -Wno-deprecated-declarations
set LNK_OPTS=-l gdi32 -l ole32 -l windowscodecs
echo Compiling %PROG_EXE_PATH% [%EXE_MODE%]...
%TDM_GPP% -DDEF_DEMO=\"roof\" %CPP_OPTS% %XCORE_FLAGS% -I %INC_DIR% -I %SRC_DIR% %SRC_FILES% -o %PROG_EXE_PATH% %LNK_OPTS%
copy /BY src\cmd\roof.bat %OUT_DIR%
copy /BY src\cmd\roof_low.bat %OUT_DIR%
if %ERRORLEVEL%==0 echo done.
%TDM_BIN%\objdump.exe -M intel -d %PROG_EXE_PATH% > %OUT_DIR%\%PROG_NAME%.txt
if not x%1==xdebug (
	if not x%1==xreldbg (
		%TDM_BIN%\strip.exe %PROG_EXE_PATH%
	)
)


@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

call tdm.bat
set PATH=%PATH%;%TDM_BIN%

set PROG_NAME=crosscore_demo

set SRC_DIR=src
set INC_DIR=inc
set BIN_DIR=bin
set TMP_DIR=tmp
set OUT_DIR=%BIN_DIR%\gcc_%TDM_MACHINE%

if not exist %OUT_DIR% mkdir %OUT_DIR%
if not exist %TMP_DIR% mkdir %TMP_DIR%
if not exist %TMP_DIR%\obj mkdir %TMP_DIR%\obj
if not exist %TMP_DIR%\obj\dbg mkdir %TMP_DIR%\obj\dbg
if not exist %TMP_DIR%\dep mkdir %TMP_DIR%\dep
if not exist %TMP_DIR%\dep\dbg mkdir %TMP_DIR%\dep\dbg

set SRCS=
for /f %%i in ('dir /b %SRC_DIR%\*.cpp') do (
	set SRCS=!SRCS! %SRC_DIR%/%%i
)

%TDM_MAK% --makefile=makefile.tdm %*

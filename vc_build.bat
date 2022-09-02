@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

if x%1==x (
	echo vc_build path_to_config.bat
	exit /B -1
) else (
	call %1
)

if x%PROJ_NAME%==x (
	set PROJ_NAME=crosscore_demo
)

if x%VC_CXX%==x (
	echo Path MSVC not specified.
	exit /B -1
)

if not exist %VC_CXX% (
	echo MSVC not found.
	exit /B -1
)


set SRC_DIR=src
set INC_DIR=inc
set BIN_DIR=bin
set TMP_DIR=tmp
set OUT_DIR=%BIN_DIR%\vcexe

set EXE_PATH="%OUT_DIR%\%PROJ_NAME%.exe"
set PDB_PATH="%OUT_DIR%\%PROJ_NAME%.pdb"
set ILK_PATH="%OUT_DIR%\%PROJ_NAME%.ilk"
set LST_PATH="%OUT_DIR%\%PROJ_NAME%.lst"

if not exist %TMP_DIR% mkdir %TMP_DIR%
if not exist %OUT_DIR% mkdir %OUT_DIR%

set SRC_FILES=
set OBJ_FILES=
for /f %%i in ('dir /b %SRC_DIR%\*.cpp') do (
	set SRC=%%i
	set SRC_FILES=!SRC_FILES! %SRC_DIR%/!SRC!
	set OBJ_FILES=!OBJ_FILES! !SRC:~0,-3!obj
)

if not x%EXT_SRC_DIR%==x (
	set EXT_SRC_FILES=
	set EXT_OBJ_FILES=
	for /f %%i in ('dir /b %EXT_SRC_DIR%\*.cpp') do (
		set SRC=%%i
		set EXT_SRC_FILES=!EXT_SRC_FILES! %EXT_SRC_DIR%/!SRC!
		set EXT_OBJ_FILES=!EXT_OBJ_FILES! !SRC:~0,-3!obj
	)
)

if exist %EXE_PATH% (
	echo Cleaning %EXE_PATH%.
	del %EXE_PATH%
)

if exist %PDB_PATH% (
	echo Cleaning %PDB_PATH%.
	del %PDB_PATH%
)

if exist %ILK_PATH% (
	echo Cleaning %ILK_PATH%.
	del %ILK_PATH%
)

if exist %LST_PATH% (
	echo Cleaning %LST_PATH%.
	del %LST_PATH%
)


set CPP_OPTS=/O2 /arch:AVX /GL /Gy /Zi /Gm- /Oi /Oy /Zc:inline  /Zc:forScope /MT /EHsc /GS- /fp:fast /DNDEBUG /D_CONSOLE  /D_UNICODE /DUNICODE
set XCORE_FLAGS=-DOGLSYS_ES=0 -DOGLSYS_CL=0 -DDRW_NO_VULKAN=1 -DXD_TSK_NATIVE=1 -DXD_XMTX_CONCAT_VEC=2
set LNK_OPTS=/DYNAMICBASE:NO "kernel32.lib" "user32.lib" "gdi32.lib" "ole32.lib"

%VC_CXX% -I %INC_DIR% %EXT_INC% %CPP_OPTS% %XCORE_FLAGS% %SRC_FILES% %EXT_SRC_FILES% /Fe%EXE_PATH% /Fd%PDB_PATH% %LNK_OPTS%

if exist src\cmd\roof.bat (
	copy /BY src\cmd\roof.bat %OUT_DIR%
)

if exist src\cmd\roof_low.bat (
	copy /BY src\cmd\roof_low.bat %OUT_DIR%
)

if exist %EXE_PATH% (
	if not xVC_DIS%==x (
		echo Generating listing...
		%VC_DIS% %EXE_PATH% > %LST_PATH%
	)
)

del /Q %OBJ_FILES% %EXT_OBJ_FILES%



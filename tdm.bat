rem https://jmeubank.github.io/tdm-gcc/download/

set TDM_DIR=%~dp0etc\tools\TDM-GCC
if not exist %TDM_DIR%\bin (
	rem set TDM_DIR=D:\Tools\Prog\TDM-GCC-64
	set TDM_DIR=D:\Tools\Prog\TDM-GCC-64_10
)

set TDM_BIN=%TDM_DIR%\bin
set TDM_GCC=%TDM_BIN%\gcc.exe
set TDM_GPP=%TDM_BIN%\g++.exe
set TDM_MAK=%TDM_BIN%\mingw32-make.exe
set TDM_WRS=%TDM_BIN%\windres.exe

for /f %%i in ('%TDM_GCC% -dumpmachine') do (
	set TDM_MACHINE=%%i
)

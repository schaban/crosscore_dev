@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
call D:\Tools\Prog\emscripten\emsdk\emsdk.bat construct_env
set NODE_SKIP_PLATFORM_CHECK=1

set OUT_HTML=bin/wgl_test.html

set SRC_FILES=
for /f %%i in ('dir /b src\*.cpp') do (
	set SRC_FILES=!SRC_FILES! src/%%i
)
emcc.bat -DOGLSYS_WEB -s WASM=1 -s SINGLE_FILE -s USE_SDL=2 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1 -std=c++11 -O3 %SRC_FILES% --pre-js src/web/opt.js --shell-file src/web/shell.html --preload-file bin/data -o %OUT_HTML% -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -s EXPORTED_FUNCTIONS=['_main','_wi_set_key_state']

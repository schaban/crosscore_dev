#!/bin/sh
OUT_HTML=bin/wgl_test.html
echo Compiling $OUT_HTML
# -s USE_WEBGL2=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s FULL_ES3=1
emcc -DOGLSYS_WEB -s WASM=1 -s SINGLE_FILE -s USE_SDL=2 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1 -std=c++11 -O3 `ls src/*.cpp` --pre-js src/web/opt.js --shell-file src/web/shell.html --preload-file bin/data -o $OUT_HTML -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -s EXPORTED_FUNCTIONS='["_main","_wi_set_key_state"]'
sed -i 's/antialias:!1/antialias:1/g' $OUT_HTML
# python3 -m http.server
# -> http://localhost:8000/

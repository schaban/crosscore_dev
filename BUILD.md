Steps to build this project on a typical GNU/Linux desktop system:
```
git clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
./get_data.sh
./get_ogl_headers.sh
make --jobs=$(nproc)
```

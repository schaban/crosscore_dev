Steps to build this project on a typical GNU/Linux desktop system:
```
git clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
./get_data.sh
./get_ogl_headers.sh
make --jobs=$(nproc)
```

***

Raspberry Pi OS:
```
(sudo apt install build-essential)
(sudo apt install libx11-dev)
git clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
./get_data.sh
./get_headers.sh
./mk_lib_links.sh
./make_gles.sh
```

***

OpenBSD/X11:
```
(pkg_add git wget gmake)
git clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
./get_data.sh
./make_openbsd.sh
```

***

GhostBSD (FreeBSD):
```
(pkg install wget gmake)
git clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
./get_data.sh
gmake
(to build with GCC: gmake CXX=g++10)
```

***

Windows with Visual Studio 2015 or later:
```
(install git)
(install Cygwin, make sure curl, tar and xz utilities are installed)
<path to git.exe> clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
get_data.bat <path to cygwin bin directory>
get_ogl_headers.bat
(Open solution file in VS and build.)
```

***

macOS (preliminary):
```
(install the command line developer tools)
git clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
./get_data.sh
./mac_build.sh
```

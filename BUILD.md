Steps to build this project on a typical GNU/Linux desktop system:
```
git clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
./get_data.sh
./get_ogl_headers.sh
make --jobs=$(nproc)
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

macOS (preliminary):
```
git clone --depth 1 https://github.com/schaban/crosscore_dev.git
cd crosscore_dev
./get_data.sh
./mac_build.sh
```


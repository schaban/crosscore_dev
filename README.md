# crosscore library

~ **crosscore** is a portable C++ library for graphics programming ~

![roof chars](https://schaban.github.io/pic/cross_chars.png)

The library itself is independent from rendering interfaces such as OpenGL or Vulkan,
instead it is conceived as a set of tools to help with creating experimental renderers.\
Model and texture resources, animation playback, collision detection are examples
of utilities that crosscore provides.

The library is essentially dependency-free and consists of a pair of source files:\
[crosscore.cpp](https://github.com/schaban/crosscore_dev/blob/main/src/crosscore.cpp)
and
[crosscore.hpp](https://github.com/schaban/crosscore_dev/blob/main/src/crosscore.hpp)

A portable OpenGL system interface is included as a separate module (see OGLSys sources),
but crosscore itself has no dependencies on this code and can be used with any similar library.

A sample OpenGL renderer is included in this repository, as well as some demo programs.
The renderer targets WebGL1 level graphics, so it's very portable albeit rather conservative
in terms of visual effects.

[~ **Here is online demo.** ~](https://schaban.github.io/crosscore_web_demo/wgl_test.html)\
It is one the included demo programs compiled to WebAssembly,
a kind of a mock-up adventure game. Here you can move the player character around,
interact with some elements in the scene and talk to NPCs
(all the text in this demo is in imaginary language).\
(Scaled-down version of this demo is
[here](https://schaban.github.io/crosscore_web_demo/wgl_test.html?small&lowq),
it is more suitable for devices such as Raspberry Pi.)

[*See also*.](https://schaban.github.io/roof/secrets.html)

***
Here is another project
that is using this library<br>
&#x2606; [**Ostinato**](https://github.com/glebnovodran/ostinato) &#x2606;

[![Ostinato](https://schaban.github.io/pic/ostinato_poster_alt.png)](https://github.com/glebnovodran/ostinato)

***
Another sample program and a [walkthrough](https://github.com/schaban/draw-walkthrough)
of implementing a very simple OpenGL renderer.
(Work in progress!)

[![draw-walkthrough](https://schaban.github.io/pic/draw_wt.jpg)](https://github.com/schaban/draw-walkthrough)

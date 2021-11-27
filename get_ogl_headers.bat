@echo off
rem https://www.khronos.org/registry/OpenGL/index_gl.php#headers
set _CSCR_=cscript.exe /nologo
if not exist inc\GL mkdir inc\GL
if not exist inc\KHR mkdir inc\KHR
set KHRGET=%_CSCR_% get_ogl_headers.js
%KHRGET% OpenGL/api/GL/glcorearb.h > inc\GL\glcorearb.h
%KHRGET% OpenGL/api/GL/glext.h > inc\GL\glext.h
%KHRGET% OpenGL/api/GL/wglext.h > inc\GL\wglext.h
%KHRGET% EGL/api/KHR/khrplatform.h > inc\KHR\khrplatform.h



uniform vec3 gpViewPos;

uniform vec3 gpHemiUp;
uniform vec3 gpHemiUpper;
uniform vec3 gpHemiLower;
uniform vec3 gpHemiParam; // exp, gain

uniform vec3 gpDiffSH[9];
uniform vec3 gpReflSH[9];

uniform vec3 gpSpecLightDir;
uniform vec4 gpSpecLightColor; // a: shadowing (1 == full)

uniform mat4 gpShadowMtx;
uniform vec4 gpShadowSize; // w, h, 1/w, 1/h
uniform vec4 gpShadowCtrl; // offs, wght, density
uniform vec4 gpShadowFade; // start, falloff

uniform vec3 gpBaseColor;
uniform vec3 gpSpecColor;
uniform vec4 gpSurfParam; // roughness, fresnel
uniform vec4 gpBumpParam; // scale, flipT, flipB
uniform vec3 gpAlphaCtrl; // lim

uniform vec4 gpBumpPatUV; // xy: offs, zw: scl
uniform vec4 gpBumpPatParam; // xy: factor

uniform vec4 gpFogColor; // rgb, density
uniform vec4 gpFogParam; // start, falloff, curveP1, curveP2

uniform vec3 gpInvWhite;
uniform vec3 gpLClrGain;
uniform vec3 gpLClrBias;
uniform vec3 gpExposure;
uniform vec3 gpInvGamma;


uniform vec4 gpPrimColor;

uniform vec4 gpFontColor;

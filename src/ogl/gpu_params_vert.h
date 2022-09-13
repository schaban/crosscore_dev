
uniform vec3 gpPosBase;
uniform vec3 gpPosScale;

uniform vec4 gpSkinMtx[JMTX_SIZE];
uniform vec4 gpSkinMap[JMAP_SIZE];

uniform vec4 gpWorld[3];

uniform mat4 gpViewProj;

uniform vec3 gpVtxHemiUp;
uniform vec3 gpVtxHemiUpper;
uniform vec3 gpVtxHemiLower;
uniform vec3 gpVtxHemiParam; // exp, gain

uniform vec4 gpTexXform; // offs.xy

uniform vec4 gpQuadVtxPos[2];
uniform vec4 gpQuadVtxTex[2];
uniform vec4 gpQuadVtxClr[4];

uniform vec4 gpFontXform;
uniform vec4 gpFontRot;

uniform vec4 gpPrimCtrl;
uniform mat4 gpInvView;

HALF vec4 sampleBaseMap(HALF vec2 uv) {
#ifdef DRW_NOBASETEX
	return vec4(0.5, 0.5, 0.5, 1.0);
#else
	HALF vec4 tex = texture2D(smpBase, uv);
	tex.rgb *= tex.rgb; // gamma 2
	return tex;
#endif
}

HALF vec4 getBaseMap(HALF vec2 uv) {
	HALF vec4 tex = sampleBaseMap(uv);
	tex.rgb *= gpBaseColor;
	return tex;
}

HALF vec2 sampleNormMap(HALF vec2 uv) {
	return texture2D(smpBump, uv).xy;
}

HALF vec3 getNormMap(HALF vec2 uv) {
	HALF vec2 ntex = sampleNormMap(uv);
	HALF vec2 nxy = (ntex - 0.5) * 2.0;
	HALF float bumpScale = gpBumpParam.x;
	nxy *= bumpScale;
	HALF vec2 sqxy = nxy * nxy;
	HALF float nz = sqrt(max(0.0, 1.0 - sqxy.x - sqxy.y));
	return vec3(nxy.x, nxy.y, nz);
}

HALF vec2 sampleNormPat(HALF vec2 uv) {
	HALF vec2 offs = gpBumpPatUV.xy;
	HALF vec2 scl = gpBumpPatUV.zw;
	return texture2D(smpBumpPat, (uv + offs) * scl).xy;
}

HALF vec2 getNormPat(HALF vec2 uv) {
	HALF vec2 npat = sampleNormPat(uv);
	HALF vec2 nxy = (npat - 0.5) * 2.0;
	HALF vec2 factor = gpBumpPatParam.xy;
	nxy *= factor;
	return nxy;
}

HALF vec3 getNormMapPat(HALF vec2 uv) {
	HALF vec2 ntex = sampleNormMap(uv);
	HALF vec2 nxy = (ntex - 0.5) * 2.0;
	HALF float bumpScale = gpBumpParam.x;
	nxy *= bumpScale;
	HALF vec2 npat = getNormPat(uv);
	nxy += npat;
	HALF vec2 sqxy = nxy * nxy;
	HALF float nz = sqrt(max(0.0, 1.0 - sqxy.x - sqxy.y));
	return vec3(nxy.x, nxy.y, nz);
}

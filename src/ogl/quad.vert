void getQuadAttrs(float ivtx, out vec2 pos, out vec2 tex, out vec4 clr) {
	int i0 = int(ivtx / 2.0);
	int i1 = int(mod(ivtx, 2.0)) * 2;
#ifdef WEBGL
	vec4 v = gpQuadVtxPos[i0];
	float x = 0.0;
	float y = 0.0;
	for (int i = 0; i < 4; ++i) { if (i == i1) { x = v[i]; break; } }
	for (int i = 0; i < 4; ++i) { if (i == i1 + 1) { y = v[i]; break; } }
	pos = vec2(x, y);
	v = gpQuadVtxTex[i0];
	for (int i = 0; i < 4; ++i) { if (i == i1) { x = v[i]; break; } }
	for (int i = 0; i < 4; ++i) { if (i == i1 + 1) { y = v[i]; break; } }
	tex = vec2(x, y);
#else
	pos = vec2(gpQuadVtxPos[i0][i1], gpQuadVtxPos[i0][i1 + 1]);
	tex = vec2(gpQuadVtxTex[i0][i1], gpQuadVtxTex[i0][i1 + 1]);
#endif
	clr = gpQuadVtxClr[int(vtxId)];
}

void main() {
	vec2 pos;
	vec2 tex;
	vec4 clr;
	getQuadAttrs(vtxId, pos, tex, clr);
	pixTex = tex;
	pixClr = clr;
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
}

void calcVtxOut(
	FULL vec4 wm[3],
	FULL vec3 vpos, HALF vec3 vnrm, vec2 vtex, vec4 vclr, float tscl, float cscl,
	out vec3 fpos, out vec3 fnrm, out vec2 ftex, out vec4 fclr
) {
	FULL vec4 hpos = vec4(vpos, 1.0);
	fpos = vec3(dot(hpos, wm[0]), dot(hpos, wm[1]), dot(hpos, wm[2]));
	HALF vec3 wmn0 = wm[0].xyz;
	HALF vec3 wmn1 = wm[1].xyz;
	HALF vec3 wmn2 = wm[2].xyz;
	HALF vec3 wnrm = normalize(vec3(dot(vnrm, wmn0), dot(vnrm, wmn1), dot(vnrm, wmn2)));
	fnrm = wnrm;
	ftex = vtex * tscl;
	fclr = vclr * cscl;
}

void calcGLPos(vec3 pos) {
	pixCPos = gpViewProj * vec4(pos, 1.0);
	gl_Position = pixCPos;
}


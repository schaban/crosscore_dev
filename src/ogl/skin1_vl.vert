void main() {
	float w0 = vtxRelPosW0.w;
	float w1 = vtxOctW1W2.z;
	float w2 = vtxOctW1W2.w;
	float w3 = 1.0 - (w0 + w1 + w2);
	vec4 wm[3];
	skinWM(vtxJnt, vec4(w0, w1, w2, w3), wm);
	vec3 vpos = vtxRelPosW0.xyz*gpPosScale + gpPosBase;
	HALF vec3 vnrm = octaDec(vtxOctW1W2.xy*2.0 - 1.0);
	const float cscl = 1.0 / float(0x7FF);
	const float tscl = 1.0 / float(0x7FF);
	calcVtxOut(wm, vpos, vnrm, vtxTex, vtxClr, tscl, cscl, pixPos, pixNrm, pixTex, pixClr);
	pixClr.rgb *= calcHemi(pixNrm);
	calcGLPos(pixPos);
}

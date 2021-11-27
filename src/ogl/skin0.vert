void main() {
	vec4 wm[3];
	skinWM(vtxJnt, vtxWgt, wm);
	HALF vec3 vnrm = octaDec(vtxOct);
	calcVtxOut(wm, vtxPos, vnrm, vtxTex, vtxClr, 1.0, 1.0, pixPos, pixNrm, pixTex, pixClr);
	calcGLPos(pixPos);
}

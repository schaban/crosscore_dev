void main() {
	HALF vec3 vnrm = octaDec(vtxOct);
	const float cscl = 1.0 / float(0x7FF);
	calcVtxOut(gpWorld, vtxPos, vnrm, vtxTex, vtxClr, 1.0, cscl, pixPos, pixNrm, pixTex, pixClr);
	pixClr.rgb *= calcHemi(pixNrm);
	calcGLPos(pixPos);
}

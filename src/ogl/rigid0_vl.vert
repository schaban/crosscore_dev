void main() {
	HALF vec3 vnrm = octaDec(vtxOct);
	calcVtxOut(gpWorld, vtxPos, vnrm, vtxTex, vtxClr, 1.0, 1.0, pixPos, pixNrm, pixTex, pixClr);
	pixClr.rgb *= calcHemi(pixNrm);
	calcGLPos(pixPos);
}

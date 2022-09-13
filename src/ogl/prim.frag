void main() {
	HALF vec4 btex = sampleBaseMap(pixTex);
	HALF vec4 c = btex * pixClr;
	c *= gpPrimColor;
	c = applyCC(c);
	gl_FragColor = c;
}

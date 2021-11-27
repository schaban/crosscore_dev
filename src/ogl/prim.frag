void main() {
	HALF vec4 btex = sampleBaseMap(pixTex);
	vec4 c = btex * pixClr;
	c = applyCC(c);
	gl_FragColor = c;
}

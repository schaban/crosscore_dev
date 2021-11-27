void main() {
	vec4 c = sampleBaseMap(pixTex);
	c *= pixClr;
	c = max(c, vec4(0.0));
	c.rgb = pow(c.rgb, gpInvGamma);
	gl_FragColor = c;
}

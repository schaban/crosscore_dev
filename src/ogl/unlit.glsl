HALF vec4 unlit() {
	HALF vec4 clr = getBaseMap(pixTex);
	HALF vec3 vc = pixClr.rgb;
	clr.rgb *= vc;
	return clr;
}


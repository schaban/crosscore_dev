vec4 hemiBasic() {
	HALF vec4 clr = getBaseMap(pixTex);
	HALF vec3 vc = pixClr.rgb;
	clr.rgb *= vc;
	clr.rgb *= calcHemi(pixNrm);
	return clr;
}


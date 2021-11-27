vec4 hemiBumpPat() {
	vec3 nrm = calcBumpNrmPat();
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	clr.rgb *= calcHemi(nrm);
	return clr;
}


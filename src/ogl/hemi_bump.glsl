vec4 hemiBump() {
	vec3 nrm = calcBumpNrm();
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	clr.rgb *= calcHemi(nrm);
	return clr;
}


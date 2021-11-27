vec4 hemiSpecBump() {
	vec3 nrm = calcBumpNrm();
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	vec3 ldiff = calcHemi(nrm);
	vec3 lspec = calcBasicSpecWithBaseMask(clr, nrm);
	clr.rgb *= ldiff;
	clr.rgb += lspec;
	return clr;
}


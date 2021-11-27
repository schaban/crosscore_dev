vec4 hemiSpecBumpPat() {
	vec3 nrm = calcBumpNrmPat();
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	vec3 ldiff = calcHemi(nrm);
	vec3 lspec = calcBasicSpecWithBaseMask(clr, nrm);
	clr.rgb *= ldiff;
	clr.rgb += lspec;
	return clr;
}


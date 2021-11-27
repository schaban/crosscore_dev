vec4 hemiSpecBasic() {
	vec3 nrm = normalize(pixNrm);
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	vec3 ldiff = calcHemi(pixNrm);
	vec3 lspec = calcBasicSpecWithBaseMask(clr, nrm);
	clr.rgb *= ldiff;
	clr.rgb += lspec;
	return clr;
}


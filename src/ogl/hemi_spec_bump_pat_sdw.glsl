vec4 hemiSpecBumpPatShadow() {
	vec3 nrm = calcBumpNrmPat();
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	float sdw = calcShadowVal();
	vec3 ldiff = calcHemi(nrm);
	vec3 lspec = calcBasicSpecWithBaseMask(clr, nrm);
	lspec = mix(lspec, lspec * (1.0 - sdw), gpSpecLightColor.a);
	clr.rgb *= ldiff;
	clr.rgb += lspec;
	clr.rgb = mix(clr.rgb, vec3(0.0), sdw * getShadowDensity());
	return clr;
}


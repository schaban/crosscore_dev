vec4 hemiBumpPatShadow() {
	vec3 nrm = calcBumpNrmPat();
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	clr.rgb *= calcHemi(nrm);
	float sdw = calcShadowVal();
	clr.rgb = mix(clr.rgb, vec3(0.0), sdw * getShadowDensity());
	return clr;
}


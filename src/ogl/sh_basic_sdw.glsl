vec4 shBasicShadow() {
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	clr.rgb *= diffSH(pixNrm);
	float sdw = calcShadowVal();
	clr.rgb = mix(clr.rgb, vec3(0.0), sdw * getShadowDensity());
	return clr;
}


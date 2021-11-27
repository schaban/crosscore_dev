HALF vec4 unlitShadow() {
	HALF vec4 clr = getBaseMap(pixTex);
	HALF vec3 vc = pixClr.rgb;
	clr.rgb *= vc;
	HALF float sdw = calcShadowVal();
	clr.rgb = mix(clr.rgb, vec3(0.0), sdw * getShadowDensity());
	return clr;
}


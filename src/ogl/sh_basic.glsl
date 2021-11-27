vec4 shBasic() {
	vec4 clr = getBaseMap(pixTex);
	clr.rgb *= pixClr.rgb;
	clr.rgb *= diffSH(pixNrm);
	return clr;
}


HALF vec3 calcHemi(HALF vec3 nrm) {
#ifdef VTX_SHADER
	HALF vec3 hemiUp = gpVtxHemiUp;
	HALF vec3 hemiLower = gpVtxHemiLower;
	HALF vec3 hemiUpper = gpVtxHemiUpper;
	HALF float hemiExp = gpVtxHemiParam.x;
	HALF float hemiGain = gpVtxHemiParam.y;
#else
	HALF vec3 hemiUp = gpHemiUp;
	HALF vec3 hemiLower = gpHemiLower;
	HALF vec3 hemiUpper = gpHemiUpper;
	HALF float hemiExp = gpHemiParam.x;
	HALF float hemiGain = gpHemiParam.y;
#endif
	HALF float hemiVal = 0.5 * (dot(nrm, hemiUp) + 1.0);
	hemiVal = clamp(pow(hemiVal, hemiExp) * hemiGain, 0.0, 1.0);
	HALF vec3 hemi = mix(hemiLower, hemiUpper, hemiVal);
	return hemi;
}




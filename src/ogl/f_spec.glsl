HALF float calcSpecLightValGGX(vec3 pos, HALF vec3 nrm, HALF float rough, HALF float fresnel) {
	vec3 vdir = normalize(pos - gpViewPos);
	//vec3 refl = reflect(vdir, nrm);
	vec3 lvec = -gpSpecLightDir;
	vec3 hvec = normalize(lvec - vdir);
	HALF float nv = max(0.0, dot(nrm, -vdir));
	HALF float nl = max(0.0, dot(nrm, lvec));
	HALF float nh = max(0.0, dot(nrm, hvec));
	HALF float lh = max(0.0, dot(lvec, hvec));
	HALF float r2 = rough * rough;
	HALF float r4 = r2 * r2;

	HALF float dv = nh*nh * (r4 - 1.0) + 1.0;
	HALF float ds = r4 / (dv*dv*3.141593);
	HALF float hr2 = r2 * 0.5;
	HALF float tnl = nl*(1.0 - hr2) + hr2;
	HALF float tnv = nv*(1.0 - hr2) + hr2;
	HALF float val = (nl*ds) / (tnl*tnv);

	HALF float sch = 1.0 - lh;
	HALF float sch2 = sch * sch;
	HALF float sch3 = sch2 * sch;
	HALF float sch5 = sch2 * sch3;
	HALF float fr = fresnel + (1.0 - fresnel) * sch5;

	return val * fr;
}

HALF float calcSpecLightValBlinn(vec3 pos, HALF vec3 nrm, HALF float rough, HALF float fresnel) {
	HALF vec3 vdir = normalize(pos - gpViewPos);
	HALF vec3 lvec = -gpSpecLightDir;
	HALF vec3 hvec = normalize(lvec - vdir);
	HALF float nv = max(0.0, dot(nrm, -vdir));
	HALF float nh = max(0.0, dot(nrm, hvec));
	HALF float r2 = rough * rough;
	HALF float r4 = r2 * r2;

	HALF float val = pow(nh, max(0.001, 2.0/r4 + 4.0));

	HALF float sch = 1.0 - nv;
	HALF float sch2 = sch * sch;
	HALF float sch3 = sch2 * sch;
	HALF float fr = fresnel + (1.0 - fresnel) * sch3;

	return val * fr;
}

HALF float calcSpecLightVal(vec3 pos, HALF vec3 nrm, HALF float rough, HALF float fresnel) {
	return calcSpecLightValGGX(pos, nrm, rough, fresnel);
}

HALF vec3 calcSpec(vec3 pos, HALF vec3 nrm, HALF float rough, HALF float fresnel) {
	HALF vec3 specLClr = gpSpecLightColor.rgb;
	return specLClr * calcSpecLightVal(pos, nrm, rough, fresnel);
}

vec3 calcBasicSpecWithBaseMask(HALF vec4 baseClr, HALF vec3 nrm) {
	HALF float rough = gpSurfParam.x;
	HALF float fresnel = gpSurfParam.y;
	HALF vec3 spec = calcSpec(pixPos, nrm, rough, fresnel);
	spec *= baseClr.a;
	HALF float vcSpecMask = pixClr.a;
	spec *= vcSpecMask;
	HALF vec3 mtlSpecClr = gpSpecColor;
	return spec * mtlSpecClr;
}

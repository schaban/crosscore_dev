void calcTangent(vec3 pos, vec3 nrm, vec2 uv, out vec3 tng, out vec3 btg) {
	vec3 npos = normalize(pos);
	vec3 s = dFdx(npos);
	vec3 t = dFdy(npos);
	vec3 v1 = cross(t, nrm);
	vec3 v2 = cross(nrm, s);
	vec2 uvdx = dFdx(uv);
	vec2 uvdy = dFdy(uv);
	tng = v1*uvdx.x + v2*uvdy.x;
	btg = v1*uvdx.y + v2*uvdy.y;
	float tscl = 1.0 / sqrt(max(dot(tng, tng), dot(btg, btg)));
	tng *= tscl;
	btg *= tscl;
}

vec3 calcBitangent(vec3 nrm, vec3 tng) {
	return normalize(cross(tng, nrm));
}

vec3 calcBumpNrm() {
	vec3 nrm = normalize(pixNrm);
	vec3 tng;
	vec3 btg;
	calcTangent(pixPos, nrm, pixTex, tng, btg);
	float sclT = gpBumpParam.y;
	float sclB = gpBumpParam.z;
	vec3 nmap = getNormMap(pixTex);
	vec3 nn = normalize(nmap.x*tng*sclT + nmap.y*btg*sclB + nmap.z*nrm);
	return nn;
}

vec3 calcBumpNrmPat() {
	vec3 nrm = normalize(pixNrm);
	vec3 tng;
	vec3 btg;
	calcTangent(pixPos, nrm, pixTex, tng, btg);
	float sclT = gpBumpParam.y;
	float sclB = gpBumpParam.z;
	vec3 nmap = getNormMapPat(pixTex);
	vec3 nn = normalize(nmap.x*tng*sclT + nmap.y*btg*sclB + nmap.z*nrm);
	return nn;
}


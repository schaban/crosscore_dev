void main() {
	FULL vec3 pos = vtxPos.xyz;
	HALF vec3 nrm = vec3(0.0, 0.0, 1.0);
	HALF vec4 clr = vtxClr;
	float primMode = gpPrimCtrl.x;
	if (primMode == 1.0) {
		/* sprite */
		HALF float rot = vtxPrm.w;
		HALF float s = sin(rot);
		HALF float c = cos(rot);
		HALF vec2 offs = vtxPrm.xy;
		HALF float scl = vtxPos.w;
		HALF float rx = offs.x*c - offs.y*s;
		HALF float ry = offs.x*s + offs.y*c;
		FULL vec4 rv = vec4(rx, ry, 0.0, 0.0);
		pos += (gpInvView * rv).xyz * scl;
	} else {
		/* poly */
		HALF vec2 oct = vtxPrm.xy;
		nrm = octaDec(oct);
		FULL vec4 wm0 = gpWorld[0];
		FULL vec4 wm1 = gpWorld[1];
		FULL vec4 wm2 = gpWorld[2];
		FULL vec4 hpos = vec4(pos, 1.0);
		pos = vec3(dot(hpos, wm0), dot(hpos, wm1), dot(hpos, wm2));
		HALF vec3 hwm0 = wm0.xyz;
		HALF vec3 hwm1 = wm1.xyz;
		HALF vec3 hwm2 = wm2.xyz;
		nrm = normalize(vec3(dot(nrm, hwm0), dot(nrm, hwm1), dot(nrm, hwm2)));
		clr.rgb *= calcHemi(nrm);
	}
	pixPos = pos;
	pixNrm = nrm;
	pixTex = vtxTex.xy;
	pixClr = clr;
	calcGLPos(pos);
}

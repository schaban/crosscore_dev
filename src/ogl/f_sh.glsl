void SH3(out HALF float sh[8], HALF vec3 v) {
	HALF float x = v.x;
	HALF float y = v.y;
	HALF float z = v.z;
	HALF float zz = z*z;
	HALF float tmp, s0 = y, s1, c0 = x, c1;
	sh[2-1] = z*0.4886025190353394;
	sh[6-1] = zz*0.9461746811866760 + -0.3153915703296661;
	tmp = -0.4886025190353394;
	sh[1-1] = tmp * s0;
	sh[3-1] = tmp * c0;
	tmp = z*-1.0925484895706177;
	sh[5-1] = tmp * s0;
	sh[7-1] = tmp * c0;
	s1 = x*s0 + y*c0;
	c1 = x*c0 - y*s0;
	tmp = 0.5462742447853088;
	sh[4-1] = tmp * s1;
	sh[8-1] = tmp * c1;
}

vec3 evalSH3(HALF vec3 v, HALF vec3 shc[9]) {
	HALF float vsh[8];
	SH3(vsh, v);
	HALF vec3 c = shc[0];
	c += shc[1] * vsh[1-1];
	c += shc[2] * vsh[2-1];
	c += shc[3] * vsh[3-1];
	c += shc[4] * vsh[4-1];
	c += shc[5] * vsh[5-1];
	c += shc[6] * vsh[6-1];
	c += shc[7] * vsh[7-1];
	c += shc[8] * vsh[8-1];
	return max(c, 0.0);
}

HALF vec3 diffSH(HALF vec3 nrm) {
	return evalSH3(nrm, gpDiffSH);
}

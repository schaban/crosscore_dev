HALF float ease(HALF vec2 cp, HALF float t) {
	HALF float tt = t*t;
	HALF float ttt = tt*t;
	HALF float b1 = dot(vec3(ttt, tt, t), vec3(3.0, -6.0, 3.0));
	HALF float b2 = dot(vec2(ttt, tt), vec2(-3.0, 3.0));
	return dot(vec3(cp, 1.0), vec3(b1, b2, ttt));
}

HALF float fog() {
	FULL float d = distance(pixPos, gpViewPos);
	HALF float start = gpFogParam.x;
	HALF float falloff = gpFogParam.y;
	HALF float hd = d;
	HALF float t = max(0.0, min((hd - start) * falloff, 1.0));
	return ease(gpFogParam.zw, t);
}

HALF vec4 exposure(HALF vec4 c) {
	HALF vec3 e = gpExposure;
	if (all(greaterThan(e, vec3(0.0)))) {
		c.rgb = 1.0 - exp(c.rgb * -e);
	}
	return c;
}

HALF vec3 toneMap(HALF vec3 c) {
	HALF vec3 invw = gpInvWhite;
	HALF vec3 gain = gpLClrGain;
	HALF vec3 bias = gpLClrBias;
	c = (c * (1.0 + c*invw)) / (1.0 + c);
	c *= gain;
	c += bias;
	c = max(c, 0.0);
	return c;
}

HALF vec4 applyCC(HALF vec4 clr) {
	HALF vec4 c = clr;

#ifndef DRW_NOFOG
	HALF vec4 cfog = gpFogColor;
	c.rgb = mix(c.rgb, cfog.rgb, fog() * cfog.a);
#endif

#ifndef DRW_NOCC
	c.rgb = toneMap(c.rgb);

	c = exposure(c);
	c = max(c, 0.0);
	HALF vec3 invg = gpInvGamma;
	c.rgb = pow(c.rgb, invg);
#else
	c = max(c, 0.0);
	c.rgb = sqrt(c.rgb);
#endif
	return c;
}

HALF vec4 calcFinalColorOpaq(HALF vec4 c) {
	c = applyCC(c);
	c.a = 1.0;
	return c;
}

HALF vec4 calcFinalColorSemi(HALF vec4 c) {
	c = applyCC(c);
	return c;
}

HALF vec4 calcFinalColorLimit(HALF vec4 c) {
	c = applyCC(c);
	HALF float lim = gpAlphaCtrl.x;
	c.a = lim < 0.0 ? c.a : c.a < lim ? 0.0 : 1.0;
	return c;
}

HALF vec4 calcFinalColorDiscard(HALF vec4 c) {
	c = applyCC(c);
	HALF float lim = gpAlphaCtrl.x;
	if (c.a < lim) discard;
	return c;
}


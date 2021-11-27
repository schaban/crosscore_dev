FULL vec4 encodeShadowVal(FULL float z) {
	FULL vec4 enc = fract(vec4(1.0, float(0x100), float(0x10000), float(0x1000000)) * z);
	enc.xyz -= enc.yzw * (1.0 / float(0x100));
	return enc;
}

FULL float decodeShadowVal(FULL vec4 enc) {
	return dot(enc, vec4(1.0, 1.0 / float(0x100), 1.0 / float(0x10000), 1.0 / float(0x1000000)));
//	return dot(enc.xy, vec2(1.0, 1.0 / float(0x100)));
}

FULL float sampleShadowMap(vec2 uv) {
	FULL vec4 enc = texture2D(smpShadow, uv);
	return decodeShadowVal(enc);
}

HALF float getShadowDensity() {
	return gpShadowCtrl.z;
}

FULL float calcShadowVal1(FULL vec4 spos) {
	FULL vec3 tpos = spos.xyz / spos.w;
	FULL vec2 uv = tpos.xy;
	FULL float sz = sampleShadowMap(uv);
	FULL float offs = gpShadowCtrl.x;
	FULL float wght = gpShadowCtrl.y;
	sz += offs;
	return wght > 0.0 ? clamp((tpos.z - sz) * wght, 0.0, 1.0) : step(sz, tpos.z);
}

HALF float calcShadowVal2(FULL vec4 spos) {
	FULL vec3 tpos = spos.xyz / spos.w;
	HALF vec2 uv00 = tpos.xy;
	HALF vec2 siwh = gpShadowSize.zw;
	HALF vec2 uv11 = tpos.xy + siwh;
	HALF float s00 = sampleShadowMap(vec2(uv00.x, uv00.y));
	HALF float s01 = sampleShadowMap(vec2(uv00.x, uv11.y));
	HALF float s10 = sampleShadowMap(vec2(uv11.x, uv00.y));
	HALF float s11 = sampleShadowMap(vec2(uv11.x, uv11.y));
	HALF vec4 sv = vec4(s00, s01, s10, s11);
	HALF vec4 zv = tpos.zzzz;

	HALF float offs = gpShadowCtrl.x;
	HALF float wght = gpShadowCtrl.y;

	sv += vec4(offs);
	HALF vec4 z = wght > 0.0
	              ? clamp((zv - sv) * wght, 0.0, 1.0)
	              : step(sv, zv);

	HALF vec2 swh = gpShadowSize.xy;
	HALF vec2 t = fract(uv00 * swh);
	HALF vec2 v = mix(z.xy, z.zw, t.x);
	return mix(v.x, v.y, t.y);
}

float calcShadowVal3(vec4 spos) {
	vec3 tpos = spos.xyz / spos.w;
	vec2 r = gpShadowSize.zw;
	vec2 uv00 = tpos.xy;
	vec2 uv_1_1 = uv00 - r;
	vec2 uv11 = uv00 + r;

	float s_1_1 = sampleShadowMap(vec2(uv_1_1.x, uv_1_1.y));
	float s_10 = sampleShadowMap(vec2(uv_1_1.x, uv00.y));
	float s_11 = sampleShadowMap(vec2(uv_1_1.x, uv11.y));

	float s0_1 = sampleShadowMap(vec2(uv00.x, uv_1_1.y));
	float s00 = sampleShadowMap(vec2(uv00.x, uv00.y));
	float s01 = sampleShadowMap(vec2(uv00.x, uv11.y));

	float s1_1 = sampleShadowMap(vec2(uv11.x, uv_1_1.y));
	float s10 = sampleShadowMap(vec2(uv11.x, uv00.y));
	float s11 = sampleShadowMap(vec2(uv11.x, uv11.y));

	float offs = gpShadowCtrl.x;
	float wght = gpShadowCtrl.y;

	vec3 sv = vec3(s_1_1, s_10, s_11) + vec3(offs);
	vec3 z_1 = wght > 0.0
	           ? clamp((tpos.zzz - sv) * wght, 0.0, 1.0)
	           : step(sv, tpos.zzz);

	sv = vec3(s0_1, s00, s01) + vec3(offs);
	vec3 z0 = wght > 0.0
	           ? clamp((tpos.zzz - sv) * wght, 0.0, 1.0)
	           : step(sv, tpos.zzz);

	sv = vec3(s1_1, s10, s11) + vec3(offs);
	vec3 z1 = wght > 0.0
	           ? clamp((tpos.zzz - sv) * wght, 0.0, 1.0)
	           : step(sv, tpos.zzz);

	vec2 t = fract(uv00 * gpShadowSize.xy);

	vec3 zv = (z_1 + z0) + (z1 - z_1)*t.x;
	return ((zv.x + zv.y) + (zv.z - zv.x)*t.y) / 4.0;
}


float calcShadowVal4(vec4 spos) {
	vec3 tpos = spos.xyz / spos.w;
	vec2 r = gpShadowSize.zw;
	vec2 uv00 = tpos.xy;
	vec2 uv_1_1 = uv00 - r;
	vec2 uv11 = uv00 + r;
	vec2 uv_2_2 = uv00 - (r + r);

	float s_2_2 = sampleShadowMap(vec2(uv_2_2.x, uv_2_2.y));
	float s_2_1 = sampleShadowMap(vec2(uv_2_2.x, uv_1_1.y));
	float s_20 = sampleShadowMap(vec2(uv_2_2.x, uv00.y));
	float s_21 = sampleShadowMap(vec2(uv_2_2.x, uv11.y));

	float s_1_2 = sampleShadowMap(vec2(uv_1_1.x, uv_2_2.y));
	float s_1_1 = sampleShadowMap(vec2(uv_1_1.x, uv_1_1.y));
	float s_10 = sampleShadowMap(vec2(uv_1_1.x, uv00.y));
	float s_11 = sampleShadowMap(vec2(uv_1_1.x, uv11.y));

	float s0_2 = sampleShadowMap(vec2(uv00.x, uv_2_2.y));
	float s0_1 = sampleShadowMap(vec2(uv00.x, uv_1_1.y));
	float s00 = sampleShadowMap(vec2(uv00.x, uv00.y));
	float s01 = sampleShadowMap(vec2(uv00.x, uv11.y));

	float s1_2 = sampleShadowMap(vec2(uv11.x, uv_2_2.y));
	float s1_1 = sampleShadowMap(vec2(uv11.x, uv_1_1.y));
	float s10 = sampleShadowMap(vec2(uv11.x, uv00.y));
	float s11 = sampleShadowMap(vec2(uv11.x, uv11.y));

	float offs = gpShadowCtrl.x;
	float wght = gpShadowCtrl.y;

	vec4 sv = vec4(s_2_2, s_1_2, s0_2, s1_2);
	sv += vec4(offs);
	vec4 z_2 = wght > 0.0
	           ? clamp((tpos.zzzz - sv) * wght, 0.0, 1.0)
	           : step(sv, tpos.zzzz);

	sv = vec4(s_2_1, s_1_1, s0_1, s1_1);
	sv += vec4(offs);
	vec4 z_1 = wght > 0.0
	           ? clamp((tpos.zzzz - sv) * wght, 0.0, 1.0)
	           : step(sv, tpos.zzzz);

	sv = vec4(s_20, s_10, s00, s10);
	sv += vec4(offs);
	vec4 z0 = wght > 0.0
	          ? clamp((tpos.zzzz - sv) * wght, 0.0, 1.0)
	          : step(sv, tpos.zzzz);

	sv = vec4(s_21, s_11, s01, s11);
	sv += vec4(offs);
	vec4 z1 = wght > 0.0
	          ? clamp((tpos.zzzz - sv) * wght, 0.0, 1.0)
	          : step(sv, tpos.zzzz);

	vec2 t = fract(uv00 * gpShadowSize.xy);
	vec4 zv = z0 + z_1 + z_2*(1.0-t.y) + z1*t.y;
	return ((zv.x + zv.y + zv.z) + (zv.w - zv.x)*t.x) / 9.0;
}

FULL float calcShadowValR(FULL vec4 spos) {
	FULL vec3 tpos = spos.xyz / spos.w;
	FULL vec2 uv = tpos.xy;
	FULL float sz = sampleShadowMap(uv);

	HALF float rot = 3.141593 * (1.0 + fract(pixTex.x)*fract(pixTex.y));
	HALF vec2 ruv = vec2(sin(rot), cos(rot));
	ruv *= -gpShadowSize.zw;
	ruv += uv;
	FULL float rsz = sampleShadowMap(ruv);

	FULL float offs = gpShadowCtrl.x;
	FULL float wght = gpShadowCtrl.y;

	FULL vec2 sv = vec2(sz, rsz);
	sv += vec2(offs);
	FULL vec2 z = wght > 0.0
	              ? clamp((tpos.zz - sv) * wght, 0.0, 1.0)
	              : step(sv, tpos.zz);
	return (z.x + z.y) * 0.5;
}

FULL vec4 calcShadowPos(FULL vec3 wpos) {
	return gpShadowMtx * vec4(wpos, 1.0);
}

FULL float calcShadowVal() {
	FULL vec4 spos = calcShadowPos(pixPos);
#ifdef GL_ES
	FULL float val = calcShadowVal2(spos);
#else
	FULL float val = calcShadowVal3(spos);
#endif
	FULL float start = gpShadowFade.x;
	FULL float falloff = gpShadowFade.y;
	FULL float d = distance(pixPos, gpViewPos);
	FULL float t = 1.0 - max(0.0, min((d - start) * falloff, 1.0));
	return val * t;
}


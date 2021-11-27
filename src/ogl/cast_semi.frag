void main() {
	vec2 uv = pixTex;
	vec4 baseTex = sampleBaseMap(uv);
	float lim = gpAlphaCtrl.x;
	if (baseTex.a < lim) discard;
	gl_FragColor = encodeShadowVal(pixCPos.z / pixCPos.w);
}

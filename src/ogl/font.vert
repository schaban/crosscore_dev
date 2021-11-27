void main() {
	vec2 pos = vtxPos;
	vec2 offs = gpFontXform.xy;
	vec2 scl = gpFontXform.zw;
	pos *= scl;
	pos += offs;
	float x = dot(pos, gpFontRot.xy);
	float y = dot(pos, gpFontRot.zw);
	gl_Position = vec4(x, y, 0.0, 1.0);
}

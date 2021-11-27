HALF vec3 octaDec(HALF vec2 oct) {
	HALF vec2 xy = oct;
	HALF vec2 a = abs(xy);
	HALF float z = 1.0 - a.x - a.y;
	HALF vec2 s = vec2(xy.x < 0.0 ? -1.0 : 1.0, xy.y < 0.0 ? -1.0 : 1.0);
	xy = mix(xy, (vec2(1.0) - a.yx) * s, vec2(z < 0.0));
	HALF vec3 n = vec3(xy, z);
	return normalize(n);
}


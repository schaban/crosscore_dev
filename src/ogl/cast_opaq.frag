void main() {
	gl_FragColor = encodeShadowVal(pixCPos.z / pixCPos.w);
}

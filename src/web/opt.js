const pageParams = new URLSearchParams(window.location.search);

Module.arguments = [
	"-demo:roof",
	"-draw:ogl",
	"-w:900", "-h:600",
	"-nwrk:0",
	"-adapt:1",
	"-data:bin/data",
	"-glsl_echo:1"
];

if (pageParams.get("small") !== null) {
	Module.arguments.push("-w:480");
	Module.arguments.push("-h:320");
}

if (pageParams.get("lowq") !== null) {
	Module.arguments.push("-lowq:1");
	Module.arguments.push("-smap:512");
	Module.arguments.push("-bump:0");
	Module.arguments.push("-spec:0");
	Module.arguments.push("-vl:1");
} else {
	Module.arguments.push("-lowq:0");
	Module.arguments.push("-smap:2048");
	Module.arguments.push("-spersp:1");
	Module.arguments.push("-bump:1");
	Module.arguments.push("-spec:1");
}

if (pageParams.get("shavian") !== null) {
	Module.arguments.push("-font:etc/font_x.xgeo");
}

if (pageParams.get("lot") !== null) {
	Module.arguments.push("-demo:lot");
	Module.arguments.push("-mode:1");
}

var btns = document.getElementsByClassName("btn");
if (btns !== null) {
	const ua = navigator.userAgent;
	if (ua.match(/Android/i) || ua.match(/iPhone/i) || ua.match(/iPad/i)) {
		Array.prototype.filter.call(btns, function(btn) {
			btn.onmousedown = "";
			btn.onmouseup = "";
		});
	}
	if (ua.match(/Android/i) || ua.match(/iPhone/i)) {
		Array.prototype.filter.call(btns, function(btn) {
			btn.style.fontSize = "24pt";
			btn.style.padding = "0 40px";
		});
	}
}

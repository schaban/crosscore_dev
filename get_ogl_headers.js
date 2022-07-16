var req = new ActiveXObject("MSXML2.XMLHTTP.6.0");
var url = "https://registry.khronos.org/" + WScript.Arguments(0);
var res = null;
WScript.StdErr.WriteLine("Downloading " + url + "...");
try {
	req.Open("GET", url, false);
	req.Send();
	res = req.ResponseText;
} catch (e) {
	WScript.StdErr.WriteLine(e.description);
}
if (res) {
	WScript.StdErr.WriteLine("Done.");
	WScript.Echo(res);
} else {
	WScript.StdErr.WriteLine("Failed.");
}


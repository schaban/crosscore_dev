var req = new ActiveXObject("WinHttp.WinHttpRequest.5.1");
var url = "http://www.khronos.org/registry/" + WScript.Arguments(0);
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


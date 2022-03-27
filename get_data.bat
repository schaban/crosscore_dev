@echo off

set _CURL_=curl
set _TAR_=tar
if not x%1==x (
	set PATH=%1
	set _CURL_=%1\curl.exe
	set _TAR_=%1\tar.exe
)

set XDATA_ARC=xcore_web.tar.xz
set XDATA_URL=https://schaban.github.io/crosscore_web_demo/%XDATA_ARC%
set XDATA_TMP=xcore_web

if not exist bin mkdir bin
if not exist %XDATA_TMP% mkdir %XDATA_TMP%

%_CURL_% %XDATA_URL% -o %XDATA_TMP%/%XDATA_ARC%
%_TAR_% -xvJf %XDATA_TMP%/%XDATA_ARC% %XDATA_TMP%/bin/data
move /Y %XDATA_TMP%\bin\data bin
rmdir /Q /S %XDATA_TMP%


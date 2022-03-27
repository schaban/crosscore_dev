@echo off
set XDATA_ARC=xcore_web.tar.xz
set XDATA_URL=https://schaban.github.io/crosscore_web_demo/%XDATA_ARC%
set XDATA_TMP=xcore_web

if not exist bin mkdir bin
if not exist %XDATA_TMP% mkdir %XDATA_TMP%

curl %XDATA_URL% -o %XDATA_TMP%/%XDATA_ARC%
tar -xvJf %XDATA_TMP%/%XDATA_ARC% %XDATA_TMP%/bin/data
move /Y %XDATA_TMP%\bin\data bin
rmdir /Q /S %XDATA_TMP%


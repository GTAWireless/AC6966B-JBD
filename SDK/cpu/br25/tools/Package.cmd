@echo off

echo Package Tool V2.0.0 By SQ
chcp 936

::Customer[Project]-主控IC(设备名Bluetooth 校验码5721-256F62A6){负责人SQ}_2012091611
set "release_file_name="
set "timedata="
set "YIname=YIname.txt"
set "CheckCodeOld="

::设置配置文件名称
set "user_file=Package.ini"


set /a a=1
echo %a%.读取配置文件
setlocal enabledelayedexpansion
for /f "tokens=1,2 delims==" %%i in (./%user_file%) do (
   set %%i=%%j
)
for /f "tokens=1,2 delims==" %%i in (%OLD_CODE_FLIN%) do (
   set %%i=%%j
)
@REM for %%I in (.) do set CurrDirName=%%~nxI
@REM set "PACKAGE_DIR=%CurrDirName%"
::返回到../../../../获取sdk文件夹名称 以此名称为sdk版本号
set cur_dir=%cd%
cd %SDK_DIR%
for %%I in (.) do set CurrDirName=%%~nxI
cd %cur_dir%
set "sdk_version=%CurrDirName%"

set "PACKAGE_PATH=%PACKAGE_DIR%"

set /a a+=1
echo %a%.作者名称
echo Writer:%Writer%


set /a a+=1
echo %a%. 获取时间
set "timedata=%date:~2,2%%date:~5,2%%date:~8,2%%time:~0,2%%time:~3,2%"
echo %time%
echo time:%timedata%

set /a a+=1
echo %a%.计算校验码
%CODE_TOOL_NAME% "%FW_FILE_PATH%" > %YIname% 
set "cmd_str=type %YIname% ^| find "固件校验码""
FOR /F "usebackq" %%i IN (`%cmd_str%`) DO @set CheckCode=%%i
::截取校验码
set "CheckCode=%CheckCode:~6,13%"
::转为大写
for %%i in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do call set CheckCode=%%CheckCode:%%i=%%i%%
echo CheckCode：%CheckCode%
set "RELEASE_PATH=%RELEASE_PATH%\%CheckCode%"
::删除临时文件
if exist "%YIname%" (del %YIname%)

set /a a+=1
echo %a%.文件名
set "release_file_name=%Customer%[%Project%]-主控%IC%(设备名%Bluetooth% 校验码%CheckCode%){负责人%Writer%}_%timedata%"
echo %release_file_name%

set /a a+=1
echo %a%. 编辑log
(
echo 版本校验码：%CheckCode%^(%Project%^)旧版本校验码：%CheckCodeOld%
echo 时间：%timedata%
echo 项目名：%Project%
echo 主控：%IC%
echo 蓝牙名：%Bluetooth%
echo SVN目录：%SVN_SERVER_DIR%
echo GIT分支：%GIT_BRANCH%
echo SDK版本：%SDK_VERSION%
echo 作者：%Writer%
echo 更新内容:
echo 1.
echo.
)>%YIname%
type %LOG_FILE_NAME% >>%YIname%
del %LOG_FILE_NAME%
ren %YIname% %LOG_FILE_NAME%
if exist "%YIname%" (del %YIname%)
start /wait notepad %LOG_FILE_NAME%

set /a a+=1
echo %a%. 检测文件夹是否存在
if exist "%RELEASE_PATH%" (
	echo %CheckCode%文件夹已存
	CHOICE /C YN /M "删除请按 Y，否则按 N"
	if ERRORLEVEL 2 goto cancel_flag
	if ERRORLEVEL 1 goto delete_flag
	
) else (
	::echo 创建%CheckCode%....	
	goto create_flag
)

:cancel_flag
goto _EXIT
:delete_flag
echo 删除中.....
rd /q /s %RELEASE_PATH%
:create_flag
md "%RELEASE_PATH%"

set /a a+=1
echo %a%. 打包BINFILE、flash目录
::需要压缩的内容
(
	::echo %RELEASE_PATH%\*
	echo %PACKAGE_PATH%
	echo %LOG_FILE_NAME%
)>R.lst
::不需要压缩的内容
( 	echo Doc 
	echo doc 
)>X.lst

"%RAR_TOOL_NAME%" a -ep1 -o+ -inul -r -m5 -ibck  -x@X.lst "%RELEASE_PATH%\%release_file_name%" @R.lst
::删除文件
del R.lst 
del X.lst 
start "" "%RELEASE_PATH%" 

(
	echo CheckCodeOld=%CheckCode%
)>%OLD_CODE_FLIN%

del "%RELEASE_PATH%\..\%LOG_FILE_NAME%" 1>nul
copy "%LOG_FILE_NAME%" "%RELEASE_PATH%\..\..\%LOG_FILE_NAME%" 1>nul
copy "%LOG_FILE_NAME%" "%RELEASE_PATH%\%LOG_FILE_NAME%" 1>nul

::pause
:_EXIT
exit
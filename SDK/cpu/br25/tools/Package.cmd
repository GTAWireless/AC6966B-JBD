@echo off

echo Package Tool V2.0.0 By SQ
chcp 936

::Customer[Project]-����IC(�豸��Bluetooth У����5721-256F62A6){������SQ}_2012091611
set "release_file_name="
set "timedata="
set "YIname=YIname.txt"
set "CheckCodeOld="

::���������ļ�����
set "user_file=Package.ini"


set /a a=1
echo %a%.��ȡ�����ļ�
setlocal enabledelayedexpansion
for /f "tokens=1,2 delims==" %%i in (./%user_file%) do (
   set %%i=%%j
)
for /f "tokens=1,2 delims==" %%i in (%OLD_CODE_FLIN%) do (
   set %%i=%%j
)
@REM for %%I in (.) do set CurrDirName=%%~nxI
@REM set "PACKAGE_DIR=%CurrDirName%"
::���ص�../../../../��ȡsdk�ļ������� �Դ�����Ϊsdk�汾��
set cur_dir=%cd%
cd %SDK_DIR%
for %%I in (.) do set CurrDirName=%%~nxI
cd %cur_dir%
set "sdk_version=%CurrDirName%"

set "PACKAGE_PATH=%PACKAGE_DIR%"

set /a a+=1
echo %a%.��������
echo Writer:%Writer%


set /a a+=1
echo %a%. ��ȡʱ��
set "timedata=%date:~2,2%%date:~5,2%%date:~8,2%%time:~0,2%%time:~3,2%"
echo %time%
echo time:%timedata%

set /a a+=1
echo %a%.����У����
%CODE_TOOL_NAME% "%FW_FILE_PATH%" > %YIname% 
set "cmd_str=type %YIname% ^| find "�̼�У����""
FOR /F "usebackq" %%i IN (`%cmd_str%`) DO @set CheckCode=%%i
::��ȡУ����
set "CheckCode=%CheckCode:~6,13%"
::תΪ��д
for %%i in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do call set CheckCode=%%CheckCode:%%i=%%i%%
echo CheckCode��%CheckCode%
set "RELEASE_PATH=%RELEASE_PATH%\%CheckCode%"
::ɾ����ʱ�ļ�
if exist "%YIname%" (del %YIname%)

set /a a+=1
echo %a%.�ļ���
set "release_file_name=%Customer%[%Project%]-����%IC%(�豸��%Bluetooth% У����%CheckCode%){������%Writer%}_%timedata%"
echo %release_file_name%

set /a a+=1
echo %a%. �༭log
(
echo �汾У���룺%CheckCode%^(%Project%^)�ɰ汾У���룺%CheckCodeOld%
echo ʱ�䣺%timedata%
echo ��Ŀ����%Project%
echo ���أ�%IC%
echo ��������%Bluetooth%
echo SVNĿ¼��%SVN_SERVER_DIR%
echo GIT��֧��%GIT_BRANCH%
echo SDK�汾��%SDK_VERSION%
echo ���ߣ�%Writer%
echo ��������:
echo 1.
echo.
)>%YIname%
type %LOG_FILE_NAME% >>%YIname%
del %LOG_FILE_NAME%
ren %YIname% %LOG_FILE_NAME%
if exist "%YIname%" (del %YIname%)
start /wait notepad %LOG_FILE_NAME%

set /a a+=1
echo %a%. ����ļ����Ƿ����
if exist "%RELEASE_PATH%" (
	echo %CheckCode%�ļ����Ѵ�
	CHOICE /C YN /M "ɾ���밴 Y������ N"
	if ERRORLEVEL 2 goto cancel_flag
	if ERRORLEVEL 1 goto delete_flag
	
) else (
	::echo ����%CheckCode%....	
	goto create_flag
)

:cancel_flag
goto _EXIT
:delete_flag
echo ɾ����.....
rd /q /s %RELEASE_PATH%
:create_flag
md "%RELEASE_PATH%"

set /a a+=1
echo %a%. ���BINFILE��flashĿ¼
::��Ҫѹ��������
(
	::echo %RELEASE_PATH%\*
	echo %PACKAGE_PATH%
	echo %LOG_FILE_NAME%
)>R.lst
::����Ҫѹ��������
( 	echo Doc 
	echo doc 
)>X.lst

"%RAR_TOOL_NAME%" a -ep1 -o+ -inul -r -m5 -ibck  -x@X.lst "%RELEASE_PATH%\%release_file_name%" @R.lst
::ɾ���ļ�
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
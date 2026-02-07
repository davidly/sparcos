@echo off
setlocal

if "%1" == "nested" (
  set _sparcosruncmd=..\sparcos -h:120 ..\c_tests\sparcos.elf
)

if "%1" == "armos" (
  set _sparcosruncmd=..\..\armos\armos -h:120 ..\..\armos\bin\sparcos
)

if "%1" == "rvos" (
  set _sparcosruncmd=..\..\rvos\rvos -h:120 ..\..\rvos\linux\sparcos
)

if "%1" == "x64os" (
  set _sparcosruncmd=..\..\x64os\x64os -h:120 ..\..\x64os\bin\sparcos
)

if "%_sparcosruncmd%" == "" (
  set _sparcosruncmd=..\sparcos
)

set outputfile=test_sparcos.txt
echo %date% %time% >%outputfile%

set _elflist=primes sieve e ttt mm

( for %%a in (%_elflist%) do (
    echo test %%a
    echo test %%a >>%outputfile%
    %_sparcosruncmd% %%a >>%outputfile%
))

echo %date% %time% >>%outputfile%
dos2unix %outputfile%
diff -b baseline_%outputfile% %outputfile%

:eof


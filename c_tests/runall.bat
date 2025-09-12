@echo off
setlocal

if "%1" == "nested" (
  set _sparcosruncmd=..\sparcos -h:120 .\sparcos.elf
)

if "%1" == "armos" (
  set _sparcosruncmd=..\..\armos\armos -h:120 ..\..\armos\bin\sparcos
)

if "%1" == "rvos" (
  set _sparcosruncmd=..\..\rvos\rvos -h:120 ..\..\rvos\linux\sparcos
)

if "%1" == "m68" (
  set _sparcosruncmd=..\..\m68\m68 -h:120 ..\..\m68\sparcos\sparcos.elf
)

if "%1" == "gnu" (
  set _sparcosruncmd=..\sparcosg
)

if "%1" == "clang" (
  set _sparcosruncmd=..\sparcoscl
)

if "%_sparcosruncmd%" == "" (
  set _sparcosruncmd=..\sparcos
)

set outputfile=test_sparcos.txt
echo %date% %time% >%outputfile%

set _elflist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
             tcmp ttypes tarray trw trw2 terrno mm_old ttime fileops tpi ^
             t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm ^
             nqueens nq1d tdir fopentst lenum trename triangle fact tld ^
             esp sievesp tttsp tttusp

( for %%a in (%_elflist%) do (
    echo test %%a
    echo test %%a >>%outputfile%
    %_sparcosruncmd% %%a >>%outputfile%
))

echo test ff
echo test ff >>%outputfile%
%_sparcosruncmd% ff -i . ff.c >>%outputfile%

echo test ba
echo test ba >>%outputfile%
%_sparcosruncmd% ba TP.BAS >>%outputfile%

echo test an
echo test an >>%outputfile%
%_sparcosruncmd% an david lee >>%outputfile%

echo %date% %time% >>%outputfile%
diff baseline_%outputfile% %outputfile%

:eof


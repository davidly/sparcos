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

if "%1" == "x64os" (
  set _sparcosruncmd=..\..\x64os\x64os -h:120 ..\..\x64os\bin\sparcos
)

if "%1" == "x32os" (
  set _sparcosruncmd=..\..\x64os\x32os -h:120 ..\..\x64os\x32bin\sparcos
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

set _elflist=hidave tprintf tm tmuldiv ttt sieve e tstr tbits t tao ^
             tcmp ttypes tarray trw trw2 terrno mm_old ttime fileops tpi ^
             t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm ^
             nqueens nq1d tdir fopentst lenum trename triangle fact tld ^
             tmmap termiosf

set _folderlist=bin0 bin1 bin2 bin3 binfast

( for %%a in (%_elflist%) do (
    echo test %%a
    ( for %%f in (%_folderlist%) do (
        echo test %%f/%%a>>%outputfile%
        %_sparcosruncmd% %%f\%%a >>%outputfile%
    ))
))

set _s_elflist=esp esp7 sievesp tttsp tttusp

( for %%a in (%_s_elflist%) do (
    echo test %%a
    echo test %%a>>%outputfile%
    %_sparcosruncmd% %%a >>%outputfile%
))

echo test ff
( for %%f in (%_folderlist%) do (
    echo test %%f/ff>>%outputfile%
    %_sparcosruncmd% %%f\ff -i . ff.c >>%outputfile%
))

echo test ba
( for %%f in (%_folderlist%) do (
    echo test %%f/ba>>%outputfile%
    %_sparcosruncmd% %%f\ba TP.BAS >>%outputfile%
))

echo test an
( for %%f in (%_folderlist%) do (
    echo test %%f/an>>%outputfile%
    %_sparcosruncmd% %%f\an david lee >>%outputfile%
))

echo test tgets
( for %%f in (%_folderlist%) do (
    echo test %%f/tgets>>%outputfile%
    %_sparcosruncmd% %%f\tgets <tgets.txt >>%outputfile%
))

echo test targs
( for %%f in (%_folderlist%) do (
    echo test %%f/targs a bb ccc dddd>>%outputfile%
    %_sparcosruncmd% %%f\targs a bb ccc dddd >>%outputfile%
))

echo %date% %time% >>%outputfile%
dos2unix %outputfile%
diff -b baseline_%outputfile% %outputfile%

:eof


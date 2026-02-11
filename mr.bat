@echo off
cl /DSPARCOS /W4 /wd4996 /wd4127 /nologo sparcos.cxx sparc.cxx /I. /EHsc /DNDEBUG /GS- /GL /Ot /Ox /Ob3 /Oi /Qpar /Zi /Fa /FAs /link /OPT:REF user32.lib




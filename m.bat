@echo off
cl /DSPARCOS /nologo sparcos.cxx sparc.cxx /I. /EHsc /DDEBUG /O2 /Oi /Fa /Qpar /Zi /link /OPT:REF user32.lib



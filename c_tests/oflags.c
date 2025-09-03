#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#ifdef WATCOM
#include <malloc.h>
#include <process.h>
#include <string.h>
#include <io.h>
#endif

int main()
{
#ifdef O_CREAT
    printf( "O_CREAT: %#x\n", O_CREAT );
#endif
#ifdef O_TRUNC
    printf( "O_TRUNC: %#x\n", O_TRUNC );
#endif
#ifdef O_ASYNC
    printf( "O_ASYNC: %#x\n", O_ASYNC );
#endif
#ifdef O_BINARY
    printf( "O_BINARY: %#x\n", O_BINARY );
#endif
#ifdef O_TEXT
    printf( "O_TEXT: %#x\n", O_TEXT );
#endif
#ifdef O_TEMPORARY
    printf( "O_TEMPORARY: %#x\n", O_TEMPORARY );
#endif
#ifdef O_TMPFILE
    printf( "O_TMPFILE: %#x\n", O_TMPFILE );
#endif
#ifdef O_FSYNC
    printf( "O_FSYNC: %#x\n", O_FSYNC );
#endif
#ifdef O_RANDOM
    printf( "O_RANDOM: %#x\n", O_RANDOM );
#endif
#ifdef O_SYNC
    printf( "O_SYNC: %#x\n", O_SYNC );
#endif
#ifdef O_SEQUENTIAL
    printf( "O_SEQUENTIAL: %#x\n", O_SEQUENTIAL );
#endif
#ifdef O_NOINHERIT
    printf( "O_NOINHERIT: %#x\n", O_NOINHERIT );
#endif
#ifdef O_RDONLY
    printf( "O_RDONLY: %#x\n", O_RDONLY );
#endif
#ifdef O_WRONLY
    printf( "O_WRONLY: %#x\n", O_WRONLY );
#endif
#ifdef O_RDWR
    printf( "O_RDWR: %#x\n", O_RDWR );
#endif
#ifdef O_APPEND
    printf( "O_APPEND: %#x\n", O_APPEND );
#endif
#ifdef O_EXCL
    printf( "O_EXCL: %#x\n", O_EXCL );
#endif
#ifdef O_EXLOCK
    printf( "O_EXLOCK: %#x\n", O_EXLOCK );
#endif
#ifdef O_SHLOCK
    printf( "O_SHLOCK: %#x\n", O_SHLOCK );
#endif
#ifdef O_DIRECT
    printf( "O_DIRECT: %#x\n", O_DIRECT );
#endif
#ifdef O_DIRECTORY
    printf( "O_DIRECTORY: %#x\n", O_DIRECTORY );
#endif
#ifdef O_OBTAINDIR
    printf( "O_OBTAINDIR: %#x\n", O_OBTAINDIR );
#endif
    return 0;
} /*main*/


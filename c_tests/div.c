#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if 0
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef unsigned int uint;
typedef int bool;
enum { false, true };
#endif

typedef unsigned int uint;

#define _noinline __attribute__((noinline))

#define ab( x ) ( x < 0 ) ? ( -x ) : ( x )

_noinline void ui64_test( uint64_t ui64A, uint64_t ui64B )
{
    uint64_t ui64C = ui64A / ui64B;
    printf( "ui64 %llu / %llu: %llu\n", ui64A, ui64B, ui64C );
}

int main()
{
    printf( "app start\n" );
    fflush( stdout );
    ui64_test( (uint64_t) 28000000000000, (uint64_t) 4 );
    fflush( stdout );
    return 0;
}


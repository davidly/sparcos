#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

int fl_cl_test_ld()
{
    long double d1_1 = 1.1;
    long double d1_8 = 1.8;
    double d;
    int32_t x;

    d = floorl( d1_1 );
    x = (int32_t) d;
    printf( "floorl of 1.1: %Lf == %ld\n", d, x );

    d = ceill( d1_1 );
    x = (int32_t) d;
    printf( "ceill of 1.1: %Lf == %ld\n", d, x );

    d = floorl( -d1_8 );
    x = (int32_t) d;
    printf( "floorl of -1.8: %Lf == %ld\n", d, x );

    d = ceill( -d1_8 );
    x = (int32_t) d;
    printf( "ceill of -1.8: %Lf == %ld\n", d, x );

    return 0;
}

extern "C" int main()
{
    long double pi = 3.14159265358979323846264338327952884197169399375105820974944592307;

    char ac[100];
    sprintf( ac, "sprintf double %.20Lf\n", pi );
    printf( ac );

    printf( "long double from printf: %.20Lf\n", pi );

    long double sq = sqrtl( pi );
    printf( "sqrt of pi: %Lf\n", sq );

    long double radians = ( 30.0 * pi ) / 180.0;
    printf( "pi in radians: %Lf\n", radians );

    long double s = sinl( radians );
    printf( "sin of 30 degress is %Lf\n", s );

    long double c = cosl( radians );
    printf( "cos of 30 degrees is %Lf\n", c );

    long double t = tanl( radians );
    printf( "tan of 30 degrees is %Lf\n", t );

    char * endptr;
    long double d = strtold( "1.0", &endptr );
    long double at = atanl( d );
    printf( "atan of %Lf is %Lf\n", d, at );

    at = atan2l( 0.3, 0.2 );
    printf( "atan2 of 0.3, 0.2 is %Lf\n", at );

    c = acosl( 0.3 );
    printf( "acos of 0.3 is %Lf\n", c );

    s = asinl( 0.3 );
    printf( "asin of 0.3 is %Lf\n", s );

    d = tanhl( 2.2 );
    printf( "tanh of 2.2 is %Lf\n", s );
    
    d = logl( 0.3 );
    printf( "log of 0.3: %Lf\n", d );

    d = log10l( 300.0 );
    printf( "log10 of 300: %Lf\n", d );
    
    long double b = 0.2;
    for ( long double a = -0.5; a < 0.5; a += 0.1 )
    {
        if ( a > b )
            printf( "g," );
        if ( a >= b )
            printf( "ge," );
        if ( a == b )
            printf( "eq," );
        if ( a < b )
            printf( "l," );
        if ( a <= b )
            printf( "le," );
    }
    printf( "\n" );

    int exponent;
    long double mantissa = frexpl( pi, &exponent );
    printf( "pi has mantissa: %Lf, exponent %d\n", mantissa, exponent );

    int loops = 1000;
    long double r = 1.0;
    for ( int i = 0; i < loops; i++ )
        r *= 1.14157222;

    t = r;
    for ( int i = 0; i < loops; i++ )
        r /= 1.14157222;

    printf( "r should be 1.0: %Lf\n", r );
    printf( "  r high point %Lf\n", t );
    
    fl_cl_test_ld();

    printf( "test tld completed with great success\n" );
    return 0;
} //main


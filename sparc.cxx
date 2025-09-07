// Emulates a sparc v8 cpu. No supervisor support.
// August 24, 2025 by David Lee

#include <stdint.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

#include <djltrace.hxx>

#include "sparc.hxx"

using namespace std;
using namespace std::chrono;

#if defined( __GNUC__ ) && !defined( __APPLE__ ) && !defined( __clang__ ) // bogus warning in g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
#pragma GCC diagnostic ignored "-Wformat="
#endif

// different compilers and cpus result in different values for NAN. Often the sign bit is turned on.
static const uint64_t g_NAN = 0x7ff8000000000000;
#define MY_NAN ( * (double *) & g_NAN )

static uint32_t g_State = 0; // 32 instead of 8 bits is faster with the msft compiler

const uint32_t stateTraceInstructions = 1;
const uint32_t stateEndEmulation = 2;

bool Sparc::trace_instructions( bool t )
{
    bool previous = ( 0 != ( g_State & stateTraceInstructions ) );
    if ( t )
        g_State |= stateTraceInstructions;
    else
        g_State &= ~stateTraceInstructions;
    return previous;
} //trace_instructions

void Sparc::end_emulation() { g_State |= stateEndEmulation; }

static inline bool sign32( uint32_t l )
{
    return( 0 != ( 0x80000000 & l ) );
} //sign32

const char * Sparc::render_flags()
{
    static char flags[ 5 ] = { 0 };
    flags[ 0 ] = flag_n() ? 'N' : 'n';
    flags[ 1 ] = flag_z() ? 'Z' : 'z';
    flags[ 2 ] = flag_v() ? 'V' : 'v';
    flags[ 3 ] = flag_c() ? 'C' : 'c';
    return flags;
} //render_flags

const uint32_t fccE = 0;   // equal
const uint32_t fccL = 1;   // less than
const uint32_t fccG = 2;   // greater than
const uint32_t fccU = 3;   // unordered

static const char fcc_flags[] = { 'E', 'L', 'G', 'U' };

const char Sparc::render_fflag()
{
    assert( get_fcc() < _countof( fcc_flags ) );
    return fcc_flags[ get_fcc() ];
} //render_fflag

static const char * render_fregstr( char * reg, uint32_t r )
{
    assert( r < 32 );
    snprintf( reg, 4, "f%u", r );
    return reg;
} //render_fregstr

static const char * fregstr1( uint32_t r )
{
    static char reg[ 4 ];
    return render_fregstr( reg, r );
} //fregstr1

static const char * fregstr2( uint32_t r )
{
    static char reg[ 4 ];
    return render_fregstr( reg, r );
} //fregstr2

static const char * fregstrd( uint32_t r )
{
    static char reg[ 4 ];
    return render_fregstr( reg, r );
} //fregstrd

static const char * render_regstr( char * reg, uint32_t r )
{
    assert( r < 32 );
    if ( r < 8 )
        snprintf( reg, 4, "g%u", r );
    else if ( 14 == r )
        strcpy( reg, "sp" );
    else if ( r < 16 )
        snprintf( reg, 4, "o%u", r - 8 );
    else if ( r < 24 )
        snprintf( reg, 4, "l%u", r - 16 );
    else if ( 30 == r )
        strcpy( reg, "fp" );
    else
        snprintf( reg, 4, "i%u", r - 24 );

    return reg;
} //render_regstr

static const char * regstr1( uint32_t r )
{
    static char reg[ 4 ];
    return render_regstr( reg, r );
} //regstr1

static const char * regstr2( uint32_t r )
{
    static char reg[ 4 ];
    return render_regstr( reg, r );
} //regstr2

static const char * regstrd( uint32_t r )
{
    static char reg[ 4 ];
    return render_regstr( reg, r );
} //regstrd

static const char * condition_string( uint32_t cond )
{
    switch( cond )
    {
        case 0: return "n";    // never
        case 1: return "e";    // equal
        case 2: return "le";   // less or equal
        case 3: return "l";    // less
        case 4: return "leu";  // less or equal unsigned
        case 5: return "cs";   // carry set
        case 6: return "neg";  // negative
        case 7: return "vs";   // overflow set
        case 8: return "a";    // always
        case 9: return "ne";   // not equal
        case 10: return "g";   // greater
        case 11: return "ge";  // greater or equal
        case 12: return "gu";  // greater unsigned
        case 13: return "cc";  // carry clear == greater than or equal unsigned
        case 14: return "pos"; // positive
        case 15: return "vc";  // overflow clear
        default:
        {
            assert( false );
            return "???";
        }
    }
} //condition_string

bool Sparc::check_condition( uint32_t cond )
{
    switch( cond )
    {
        case 0: return false;                                      // never
        case 1: return flag_z();                                   // equal
        case 2: return ( flag_z() || ( flag_n() ^ flag_v() ) );    // less or equal
        case 3: return ( flag_n() ^ flag_v() );                    // less
        case 4: return ( flag_c() || flag_z() );                   // less or equal unsigned
        case 5: return flag_c();                                   // carry set
        case 6: return flag_n();                                   // negative
        case 7: return flag_v();                                   // overflow set
        case 8: return true;                                       // always
        case 9: return !flag_z();                                  // not equal
        case 10: return ! ( flag_z() || ( flag_n() ^ flag_v() ) ); // greater
        case 11: return ! ( flag_n() ^ flag_v() );                 // greater or equal
        case 12: return ! ( flag_c() || flag_z() );                // greater unsigned
        case 13: return !flag_c();                                 // carry clear == greater than or equal unsigned
        case 14: return !flag_n();                                 // positive
        case 15: return !flag_v();                                 // overflow clear
        default:
        {
            assert( false );
            return false;
        }
    }
} //check_condition

static const char * fcondition_string( uint32_t cond )
{
    switch( cond )
    {
        case 0: return "n";    // never
        case 1: return "ne";   // not equal
        case 2: return "lge";  // less or greater
        case 3: return "ul";   // unordered or less
        case 4: return "l";    // less
        case 5: return "ug";   // unordered or greater
        case 6: return "g";    // greater
        case 7: return "u";    // unordered
        case 8: return "a";    // always
        case 9: return "e";    // equal
        case 10: return "ue";  // unordered or equal
        case 11: return "ge";  // greater or equal
        case 12: return "uge"; // unordered or greater or equal
        case 13: return "le";  // less or equal
        case 14: return "ule"; // unordered or less or equal
        case 15: return "o";   // ordered
        default:
        {
            assert( false );
            return "???";
        }
    }
} //fcondition_string

bool Sparc::check_fcondition( uint32_t cond )
{
    uint32_t fcc = get_fcc();
    switch( cond ) // note that the floating point and integer conditions are quite different!
    {
        case 0: return false;                                 // never
        case 1: return ( fccE != fcc );                       // not equal
        case 2: return ( fccE == fcc || fccL == fcc );        // less or greater
        case 3: return ( fccU == fcc || fccL == fcc );        // unordered or less
        case 4: return ( fccL == fcc );                       // less
        case 5: return ( fccU == fcc || fccG == fcc );        // unordered or greater
        case 6: return ( fccG == fcc );                       // greater
        case 7: return ( fccU == fcc );                       // unordered
        case 8: return true;                                  // always
        case 9: return ( fccE == fcc );                       // equal
        case 10: return ( fccU == fcc || fccE == fcc );       // unordered or equal
        case 11: return ( fccG == fcc || fccE == fcc );       // greater or equal
        case 12: return ( fccL != fcc );                      // unordered or greater or equal
        case 13: return ( fccL == fcc || fccE == fcc );       // less or equal
        case 14: return ( fccG != fcc );                      // unordered or less or equal
        case 15: return ( fccU != fcc );                      // ordered
        default:
        {
            assert( false );
            return false;
        }
    }
} //check_fcondition

void Sparc::trace_canonical( const char * pins )
{
    uint32_t rd = opbits( 25, 5 );
    uint32_t rs1 = opbits( 14, 5 );
    uint32_t i = opbit( 13 );
    if ( i )
    {
        int32_t simm13 = sign_extend( opbits( 0, 13 ), 12 );
        tracer.Trace( "%s %s, %#x, %s\n", pins, regstr1( rs1 ), simm13, regstrd( rd ) );
    }
    else
    {
        uint32_t rs2 = opbits( 0, 5 );
        tracer.Trace( "%s %s, %s, %s\n", pins, regstr1( rs1 ), regstr2( rs2 ), regstrd( rd ) );
    }
} //trace_canonical

void Sparc::trace_shift_canonical( const char * pins )
{
    uint32_t rd = opbits( 25, 5 );
    uint32_t rs1 = opbits( 14, 5 );
    uint32_t i = opbit( 13 );
    if ( i )
    {
        int32_t simm13 = sign_extend( opbits( 0, 13 ), 12 );
        tracer.Trace( "%s %s, %#x, %s\n", pins, regstr1( rs1 ), simm13 & 0x1f, regstrd( rd ) );
    }
    else
    {
        uint32_t rs2 = opbits( 0, 5 );
        tracer.Trace( "%s %s, %s, %s\n", pins, regstr1( rs1 ), regstr2( rs2 ), regstrd( rd ) );
    }
} //trace_shift_canonical

void Sparc::trace_ld_canonical( const char * pins )
{
    uint32_t rd = opbits( 25, 5 );
    uint32_t rs1 = opbits( 14, 5 );
    uint32_t i = opbit( 13 );
    if ( i )
    {
        int32_t simm13 = sign_extend( opbits( 0, 13 ), 12 );
        tracer.Trace( "%s [%s + %#x], %s\n", pins, regstr1( rs1 ), simm13, regstrd( rd ) );
    }
    else
    {
        uint32_t rs2 = opbits( 0, 5 );
        if ( 0 == rs2 )
            tracer.Trace( "%s [%s], %s\n", pins, regstr1( rs1 ), regstrd( rd ) );
        else
            tracer.Trace( "%s [%s + %s], %s\n", pins, regstr1( rs1 ), regstr2( rs2 ), regstrd( rd ) );
    }
} //trace_ld_canonical

void Sparc::trace_st_canonical( const char * pins )
{
    uint32_t rd = opbits( 25, 5 );
    uint32_t rs1 = opbits( 14, 5 );
    uint32_t i = opbit( 13 );
    if ( i )
    {
        int32_t simm13 = sign_extend( opbits( 0, 13 ), 12 );
        tracer.Trace( "%s %s, [%s + %#x]\n", pins, regstrd( rd ), regstr1( rs1 ), simm13 );
    }
    else
    {
        uint32_t rs2 = opbits( 0, 5 );
        if ( 0 == rs2 )
            tracer.Trace( "%s %s, [%s]\n", pins, regstrd( rd ), regstr1( rs1 ) );
        else
            tracer.Trace( "%s %s, [%s + %s]\n", pins, regstrd( rd ), regstr1( rs1 ), regstr2( rs2 ) );
    }
} //trace_st_canonical

void Sparc::trace_state()
{
    static const char * previous_symbol = 0;
    uint32_t offset;
    const char * symbol_name = emulator_symbol_lookup( pc, offset );
    if ( symbol_name == previous_symbol )
        symbol_name = "";
    else
        previous_symbol = symbol_name;

    char symbol_offset[40];
    symbol_offset[ 0 ] = 0;

    if ( 0 != symbol_name[ 0 ] )
    {
        if ( 0 != offset )
            snprintf( symbol_offset, _countof( symbol_offset ), " + %x", offset );
        strcat( symbol_offset, "\n            " );
    }

    static char acregs[ 32 * 16 + 10 ]; // way too much.
    acregs[ 0 ] = 0;
    int len = 0;
    for ( int r = 0; r < 32; r++ )
        if ( 0 != Sparc_reg( r ) )
            len += snprintf( & acregs[ len ], 16, "%s:%x ", regstr1( r ), Sparc_reg( r ) );

    if ( 0 != y )
        len += snprintf( & acregs[ len ], 16, "y:%x ", y );

    tracer.Trace( "pc %8x %s%sop %8x %s %c c:%u w:%x %s=> ", pc, symbol_name, symbol_offset, opcode, render_flags(), render_fflag(), get_cwp(), wim, acregs );

    uint32_t op = opbits( 30, 2 );
    if ( 0 == op ) // format 2: sethi & branches ( Bicc, FBfcc, CBccc )
    {
        uint32_t op2 = opbits( 22, 3 );
        uint32_t imm22 = opbits( 0, 22 );
        int32_t disp22 = sign_extend( imm22, 21 );

        switch( op2 )
        {
            case 0: tracer.Trace( "unimp\n" );
            case 2: // Bicc
            {
                uint32_t a = opbit( 29 );
                uint32_t cond = opbits( 25, 4 );
                tracer.Trace( "b%s%s %#x # %#lx\n", condition_string( cond ), a ? ",a" : "", disp22 << 2, pc + ( disp22 << 2 ) );
                break;
            }
            case 4: // sethi
            {
                uint32_t rd = opbits( 25, 5 );
                if ( 0 == rd && 0 == imm22 )
                    tracer.Trace( "nop\n" );
                else
                    tracer.Trace( "sethi %#x, %s\n", imm22, regstrd( rd ) );
                break;
            }
            case 6: // FBfcc
            {
                uint32_t a = opbit( 29 );
                uint32_t cond = opbits( 25, 4 );
                tracer.Trace( "fb%s%s %#x # %#lx\n", fcondition_string( cond ), a ? ",a" : "", disp22 << 2, pc + ( disp22 << 2 ) );
                break;
            }
            case 7: // CBccc. There is no coprocessor
            {
                uint32_t a = opbit( 29 );
                uint32_t cond = opbits( 25, 4 );
                tracer.Trace( "cb%s%s %#x\n", condition_string( cond ), a ? ",a" : "", disp22 << 2 );
                break;
            }
            default:
                unhandled();
        }
    }
    else if ( 1 == op ) // format 1. call
    {
        int32_t disp30 = sign_extend( opbits( 0, 30 ), 29 );
        tracer.Trace( "call %#x\n", 4 * disp30 );
    }
    else // format 3: op=2/3. remaining instructions
    {
        uint32_t rd = opbits( 25, 5 );
        uint32_t op3 = opbits( 19, 6 );
        uint32_t rs1 = opbits( 14, 5 );
        uint32_t i = opbit( 13 );
        uint32_t rs2 =  opbits( 0, 5 );
        int32_t simm13 = sign_extend( opbits( 0, 13 ), 12 );

        if ( 2 == op ) // arithmetic, logical, shift, and remaining
        {
            switch( op3 )
            {
                case 0: { trace_canonical( "add" ); break; }
                case 1: { trace_canonical( "and" ); break; }
                case 2:
                {
                    if ( 0 == rs1 )
                        if ( i )
                            if ( 0 == simm13 )
                                tracer.Trace( "clr %s\n", regstrd( rd ) );
                            else
                                tracer.Trace( "mov %#x, %s\n", simm13, regstrd( rd ) );
                        else
                            tracer.Trace( "mov %s, %s\n", regstr2( rs2 ), regstrd( rd ) );
                    else
                        trace_canonical( "or" );
                    break;
                }
                case 3: { trace_canonical( "xor" ); break; }
                case 4: { trace_canonical( "sub" ); break; }
                case 5: { trace_canonical( "andn" ); break; }
                case 6: { trace_canonical( "orn" ); break; }
                case 7: { trace_canonical( "xnor" ); break; }
                case 8: { trace_canonical( "addx" ); break; }
                case 0xa: { trace_canonical( "umul" ); break; }
                case 0xb: { trace_canonical( "smul" ); break; }
                case 0xc: { trace_canonical( "subx" ); break; }
                case 0xe: { trace_canonical( "udiv" ); break; }
                case 0xf: { trace_canonical( "sdiv" ); break; }
                case 0x10: { trace_canonical( "addcc" ); break; }
                case 0x11: { trace_canonical( "andcc" ); break; }
                case 0x12: // orcc
                {
                    if ( 0 == rs1 && 0 == rd && !i )
                        tracer.Trace( "tst %s\n", regstr2( rs2 ) );
                    else if ( 0 == rs2 && 0 == rd && !i )
                        tracer.Trace( "tst %s\n", regstr1( rs1 ) );
                    else
                        trace_canonical( "orcc" );
                    break;
                }
                case 0x13: { trace_canonical( "xorcc" ); break; }
                case 0x14:
                {
                    if ( 0 == rd )
                        if ( i )
                            tracer.Trace( "cmp %s, %#x\n", regstr1( rs1 ), simm13 );
                        else
                            tracer.Trace( "cmp %s, %s\n", regstr1( rs1 ), regstr2( rs2 ) );
                    else
                        trace_canonical( "subcc" );
                    break;
                }
                case 0x15: { trace_canonical( "andncc" ); break; }
                case 0x16: { trace_canonical( "orncc" ); break; }
                case 0x17: { trace_canonical( "xnorcc" ); break; }
                case 0x18: { trace_canonical( "addxcc" ); break; }
                case 0x1a: { trace_canonical( "umulcc" ); break; }
                case 0x1b: { trace_canonical( "smulcc" ); break; }
                case 0x1c: { trace_canonical( "subxcc" ); break; }
                case 0x1e: { trace_canonical( "udivcc" ); break; }
                case 0x1f: { trace_canonical( "sdivcc" ); break; }
                case 0x20: { trace_canonical( "taddcc" ); break; }
                case 0x21: { trace_canonical( "tsubcc" ); break; }
                case 0x24: { trace_canonical( "mulscc" ); break; }
                case 0x25: { trace_shift_canonical( "sll" ); break; }
                case 0x26: { trace_shift_canonical( "srl" ); break; }
                case 0x27: { trace_shift_canonical( "sla" ); break; }
                case 0x28: // rdy
                {
                    if ( 0 == rs1 ) // rdy
                        tracer.Trace( "rd y, %s\n", regstrd( rd ) );
                    else if ( 0xf == rs1 ) // stbar
                        tracer.Trace( "stbar\n" );
                    else
                        unhandled();
                    break;
                }
                case 0x30: // wry, wrasr, wrpsr, wrwim, wrtbr
                {
                    if ( 0 == rd ) // wry
                        if ( i )
                            tracer.Trace( "wr %#x ^ %s, y\n", simm13, regstr1( rs1 ) );
                        else
                            tracer.Trace( "wr %s ^ %s, y\n", regstr1( rs1 ), regstr2( rs2 ) );
                    else
                        unhandled();
                    break;
                }
                case 0x34:
                {
                    uint32_t opf = opbits( 5, 9 );
                    switch( opf )
                    {
                        case 1: tracer.Trace( "fmovs %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 5: tracer.Trace( "fnegs %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 9: tracer.Trace( "fabss %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x29: tracer.Trace( "fsqrts %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x2a: tracer.Trace( "fsqrtd %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x2b: tracer.Trace( "fsqrtq %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x41: tracer.Trace( "fadds %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x42: tracer.Trace( "faddd %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x43: tracer.Trace( "faddq %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x45: tracer.Trace( "fsubs %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x46: tracer.Trace( "fsubd %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x47: tracer.Trace( "fsubq %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x49: tracer.Trace( "fmuls %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x4a: tracer.Trace( "fmuld %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x4b: tracer.Trace( "fmulq %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x4d: tracer.Trace( "fdivs %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x4e: tracer.Trace( "fdivd %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x4f: tracer.Trace( "fdivq %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x69: tracer.Trace( "fsmuld %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0x6e: tracer.Trace( "fsmulq %s, %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xc4: tracer.Trace( "fitos %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xc6: tracer.Trace( "fdtos %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xc7: tracer.Trace( "fqtos %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xc8: tracer.Trace( "fitod %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xc9: tracer.Trace( "fstod %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xcb: tracer.Trace( "fqtod %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xcc: tracer.Trace( "fitoq %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xcd: tracer.Trace( "fstoq %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xce: tracer.Trace( "fdtoq %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xd1: tracer.Trace( "fstoi %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xd2: tracer.Trace( "fdtoi %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        case 0xd3: tracer.Trace( "fqtoi %s, %s\n", fregstr2( rs2 ), fregstrd( rd ) ); break;
                        default: unhandled(); break;
                    }
                    break;
                }
                case 0x35: // fcmps, fcpmd, fpmpq and exception variants
                {
                    uint32_t opf = opbits( 5, 9 );
                    switch( opf )
                    {
                        case 0x51: tracer.Trace( "fcmps %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ) ); break;
                        case 0x52: tracer.Trace( "fcmpd %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ) ); break;
                        case 0x53: tracer.Trace( "fcmpq %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ) ); break;
                        case 0x55: tracer.Trace( "fcmpes %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ) ); break;
                        case 0x56: tracer.Trace( "fcmped %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ) ); break;
                        case 0x57: tracer.Trace( "fcmpeq %s, %s\n", fregstr1( rs1 ), fregstr2( rs2 ) ); break;
                        default: unhandled(); break;
                    }
                    break;
                }
                case 0x38: // jmpl
                {
                    if ( 0 == rd )
                        if ( i && 8 == simm13 )
                            if ( 31 == rs1 )
                                tracer.Trace( "ret\n" );
                            else if ( 15 == rs1 )
                                tracer.Trace( "retl\n" );
                            else
                                trace_canonical( "jmpl" );
                        else
                            if ( i )
                                tracer.Trace( "jmp %s + %d\n", regstr1( rs1 ), simm13 );
                            else
                                tracer.Trace( "jmp %s + %s\n", regstr1( rs1 ), regstr2( rs2 ) );
                    else if ( 15 == rd )
                        if ( i )
                            tracer.Trace( "call %s + %d\n", regstr1( rs1 ), simm13 );
                        else if ( 0 == rs2 )
                            tracer.Trace( "call %s\n", regstr1( rs1 ) );
                        else
                            tracer.Trace( "call %s + %s\n", regstr1( rs1 ), regstr2( rs2 ) );
                    else
                        trace_canonical( "jmpl" );
                    break;
                }
                case 0x3a: // tcc
                {
                    uint32_t cond = opbits( 25, 4 );
                    if ( i )
                        tracer.Trace( "t%s %#x, %s\n", condition_string( cond ), simm13, regstr1( rs1 ) );
                    else
                        tracer.Trace( "t%s %s, %s\n", condition_string( cond ), regstr1( rs1 ), regstr2( rs2 ) );
                    break;
                }
                case 0x3b: // flush
                {
                    if ( i )
                        tracer.Trace( "flush %s + %d\n", regstr1( rs1 ), simm13 );
                    else
                        tracer.Trace( "flush %s + %s\n", regstr1( rs1 ), regstr1( rs2 ) );
                    break;
                }
                case 0x3c: { trace_canonical( "save" ); break; }// save
                case 0x3d: { trace_canonical( "restore" ); break; }// restore
                default:
                {
                    tracer.Trace( "op %d, rd %d, op3 %#x, rs1 %d, rs2 %d, i %d\n", op, rd, op3, rs1, rs2, i );
                    unhandled();
                }
            }
        }
        else // 3 == op. memory
        {
            assert( 3 == op );
            switch( op3 )
            {
                case 0: { trace_ld_canonical( "ld" ); break; }
                case 1: { trace_ld_canonical( "ldub" ); break; }
                case 2: { trace_ld_canonical( "lduh" ); break; }
                case 3: { trace_ld_canonical( "ldd" ); break; }
                case 4: { trace_st_canonical( "st" ); break; }
                case 5: { trace_st_canonical( "stb" ); break; }
                case 6: { trace_st_canonical( "sth" ); break; }
                case 7: { trace_st_canonical( "std" ); break; }
                case 9: { trace_ld_canonical( "ldsb" ); break; }
                case 0xa: { trace_ld_canonical( "ldsh" ); break; }
                case 0xd: { trace_ld_canonical( "ldstub" ); break; }
                case 0xf: { trace_ld_canonical( "swap" ); break; }
                case 0x20: { trace_ld_canonical( "ldf" ); break; }
                case 0x21: // ldfsr
                {
                    if ( i )
                        tracer.Trace( "ld [%s + %#x], fsr\n", regstr1( rs1 ), simm13 );
                    else
                        tracer.Trace( "ld [%s + %s], fsr\n", regstr1( rs1 ), regstr2( rs2 ) );
                    break;
                }
                case 0x23: { trace_ld_canonical( "lddf" ); break; }
                case 0x24: { trace_st_canonical( "stf" ); break; }
                case 0x25: // stfsr
                {
                    if ( i )
                        tracer.Trace( "st fsr, [%s + %#x]\n", regstr1( rs1 ), simm13 );
                    else
                        tracer.Trace( "st fsr, [%s + %s]\n", regstr1( rs1 ), regstr2( rs2 ) );
                    break;
                }
                case 0x27: { trace_st_canonical( "stdf" ); break; }
                default:
                {
                    tracer.Trace( "op 3 op %d, rd %d, op3 %#x, rs1 %d, rs2 %d, i %d\n", op, rd, op3, rs1, rs2, i );
                    unhandled();
                }
            }
        }
    }
} //trace_state

void Sparc::handle_trap( uint32_t trap )
{
    assert( trap < 256 );

    if ( 0x90 == trap ) // linux syscall
    {
        emulator_invoke_svc( *this );
        return;
    }

    if ( 0x83 == trap ) // register window flush trap. no need to do anything because this is for free in the emulator
        return;

    if ( 0x82 == trap ) // sparc v7 software integer divide by zero. ignore for now
        return;

    if ( 5 == trap ) // overflow trap from a save instruction. could also be written in sparc assembly and installed with a trap handler.
    {
        emulator_invoke_sparc_trap5( *this );
        return;
    }

    if ( 6 == trap ) // underflow trap from a restore instruction.
    {
        emulator_invoke_sparc_trap6( *this );
        return;
    }

    tracer.Trace( "handle_trap %#x\n", trap );
    psr &= ~ ( 1 << 5 ); // set Enable Traps ET to 0

    uint32_t supervisor = get_bit32( psr, 7 );
    psr &= ~ ( 1 << 6 ); // clear previous supervisor PS
    psr |= ( supervisor << 6 ); // set previous supervisor PS if needed
    psr |= ( 1 << 7 ); // enable supervisor bit

    set_cwp( next_save_cwp( get_cwp() ) );
    Sparc_reg( 17 ) = pc;      // overwrite the record regardless of its state
    Sparc_reg( 18 ) = npc;

    tbr &= ~ ( 255 << 4 );
    tbr |= ( trap << 4 );
    npc = tbr;
} //handle_trap

template < typename T > inline uint32_t compare_floating( T a, T b )
{
    if ( a == b )
        return fccE;
    if ( a < b )
        return fccL;
    if ( a > b )
        return fccG;

    return fccU;
} //compare_floating

void Sparc::trace_fregs()
{
    if ( g_State & stateTraceInstructions )
    {
        for ( uint32_t r = 0; r < 32; r++ )
        {
            if ( 0.0 != fregs[ r ] )
                tracer.Trace( "freg[%u] = %f / %#x\n", r, fregs[ r ], * (uint32_t *) & fregs[ r ] );
            if ( ( 0 == ( r % 2 ) ) )
            {
                double d = get_dreg( r );
                if ( 0.0 != d )
                    tracer.Trace( "dreg[%u] = %lf / %#llx\n", r, d, * (uint64_t *) & d );
            }
        }
    }
} //trace_fregs

double set_double_sign( double d, bool sign )
{
    uint64_t val = sign ? ( ( * (uint64_t *) &d ) | 0x8000000000000000 ) : ( ( * (uint64_t *) &d ) & 0x7fffffffffffffff );
    return * (double *) &val;
} //set_double_sign

double do_fsub( double a, double b )
{
    if ( isinf( a ) && isinf( b ) )
    {
        if ( signbit( a ) != signbit( b ) )
            return a;
        return MY_NAN; // msft C will return -nan if this check isn't here
    }

    if ( isnan( a ) )
        return a;
    if ( isnan( b ) )
        return b;

    return a - b;
} //do_fsub

double do_fadd( double a, double b )
{
    bool ainf = isinf( a );
    bool binf = isinf( b );

    if ( ainf && binf )
    {
        if ( signbit( a ) == signbit( b ) )
            return a;
        return MY_NAN; // msft C will return -nan if this check isn't here
    }

    if ( isnan( a ) )
        return a;
    if ( isnan( b ) )
        return b;
    if ( ainf )
        return a;
    if ( binf )
        return b;

    return a + b;
} //do_fadd

double do_fmul( double a, double b )
{
    if ( isnan( a ) )
        return a;
    if ( isnan( b ) )
        return b;

    bool ainf = isinf( a );
    bool binf = isinf( b );
    bool azero = ( 0.0 == a );
    bool bzero = ( 0.0 == b );

    if ( ( ainf && bzero ) || ( azero && binf ) )
        return MY_NAN;
    if ( ainf && binf )
        set_double_sign( INFINITY, ( signbit( a ) != signbit( b ) ) );
    if ( ainf || binf )
        return set_double_sign( INFINITY, signbit( a ) != signbit( b ) );
    if ( azero || bzero )
        return set_double_sign( 0.0, signbit( a ) != signbit( b ) );

    return a * b;
} //do_fmul

double do_fdiv( double a, double b )
{
    if ( isnan( a ) )
        return a;
    if ( isnan( b ) )
        return b;

    bool ainf = isinf( a );
    bool binf = isinf( b );
    bool azero = ( 0.0 == a );
    bool bzero = ( 0.0 == b );

    if ( ( ainf && binf ) || ( azero && bzero ) )
        return MY_NAN;
    if ( ainf )
        return set_double_sign( INFINITY, signbit( a ) != signbit( b ) );
    if ( binf )
        return set_double_sign( 0.0, signbit( a ) != signbit( b ) );
    if ( azero )
        return set_double_sign( 0.0, signbit( a ) != signbit( b ) );

    return a / b;
} //do_fdiv

#ifdef _WIN32
__declspec(noinline)
#endif
void Sparc::unhandled()
{
    printf( "unhandled opcode %x\n", opcode );
    tracer.Trace( "unhandled opcode %x\n", opcode );
    emulator_hard_termination( *this, "opcode not handled:", opcode ); // there won't be a a handler for gcc apps so terminate
} //unhandled

uint64_t Sparc::run()
{
    tracer.Trace( "code at pc %x:\n", pc );
    tracer.TraceBinaryData( getmem( pc ), 128, 4 );

    uint32_t delay_instruction = 0; // 0 == no delay, 1 == execute delay, 2 == after a save/restore in a delay slot trap
    uint64_t cycles = 0; // really just an instruction count but sparc makes claims about 1c per 1i

    for ( ;; )
    {
        #ifndef NDEBUG
            if ( Sparc_reg( 14 ) > stack_top ) // 14 is sp
                emulator_hard_termination( *this, "sp is higher than the top of the stack", Sparc_reg( 14 ) );
            if ( Sparc_reg( 14 ) <= ( stack_top - stack_size ) ) // 14 is sp
                emulator_hard_termination( *this, "sp is lower than the bottom of the stack", Sparc_reg( 14 ) );
            if ( pc < base )
                emulator_hard_termination( *this, "pc is lower than memory:", pc );
            if ( pc >= ( base + mem_size - stack_size ) )
                emulator_hard_termination( *this, "pc is higher than it should be:", pc );
            if ( ( 32 != NWINDOWS ) && ( 0 != ( wim & ~( ( 1 << NWINDOWS ) - 1 ) ) ) )
                emulator_hard_termination( *this, "wim is invalid:", wim );
            if ( 0 != Sparc_reg( 0 ) )
                emulator_hard_termination( *this, "g0 isn't zero:", Sparc_reg( 0 ) );
            if ( 0 != ( pc & 3 ) ) // to avoid alignment faults
                emulator_hard_termination( *this, "the pc isn't 4-byte aligned:", pc );
        #endif

        if ( 1 == delay_instruction )   // 1 means the delay slot instruction is being executed, so don't move pc or npc
            delay_instruction++;
        else                            // 0 or 2
        {
            pc = npc;
            npc += 4;
            delay_instruction = 0;
        }

        cycles++;
        opcode = getui32( pc );

        if ( 0 != g_State )
        {
            if ( g_State & stateEndEmulation )
            {
                g_State &= ~stateEndEmulation;
                break;
            }
            if ( g_State & stateTraceInstructions )
                trace_state();
        }

        uint32_t op = opbits( 30, 2 );
        if ( 0 == op ) // format 2: sethi & branches ( Bicc, FBfcc, CBccc )
        {
            uint32_t op2 = opbits( 22, 3 );
            uint32_t imm22 = opbits( 0, 22 );
            int32_t disp22 = sign_extend( imm22, 21 );

            switch( op2 )
            {
                case 0: { handle_trap( 2 ); break; } // unimp
                case 2: // Bicc
                {
                    uint32_t a = opbit( 29 );
                    uint32_t cond = opbits( 25, 4 );
                    bool branch = check_condition( cond );
                    if ( branch )
                    {
                        npc = pc + ( disp22 << 2 );
                        if ( ! ( ( 8 == cond ) && a ) ) // delay slot instructions always executed unless it's BA (branch always) and annulled
                        {
                            delay_instruction = 1;
                            pc += 4;
                        }
                    }
                    else if ( a )
                        npc = pc + 8; // skip the annulled delay slot instruction
                    break;
                }
                case 4: // sethi
                {
                    uint32_t rd = opbits( 25, 5 );
                    if ( 0 != rd )
                        Sparc_reg( rd ) = ( imm22 << 10 );
                    break;
                }
                case 6: // FBfcc
                {
                    uint32_t a = opbit( 29 );
                    uint32_t cond = opbits( 25, 4 );
                    bool branch = check_fcondition( cond );
                    if ( branch )
                    {
                        npc = pc + ( disp22 << 2 );
                        if ( ! ( ( 8 == cond ) && a ) ) // delay slot instructions always executed unless it's BA (branch always) and annulled
                        {
                            delay_instruction = 1;
                            pc += 4;
                        }
                    }
                    else if ( a )
                        npc = pc + 8; // skip the annulled delay slot instruction
                    break;
                }
                case 7: unhandled(); // CBccc. There is no coprocessor
                default:
                    unhandled();
            }
        }
        else if ( 1 == op ) // format 1. call
        {
            int32_t disp30 = sign_extend( opbits( 0, 30 ), 29 );
            Sparc_reg( 15 ) = pc; // 15 is o7
            npc = pc + 4 * disp30; // where to branch after the delay slot instruction
            delay_instruction = 1;
            pc += 4; // point to the delay instruction
        }
        else // format 3: op=2/3. remaining instructions
        {
            uint32_t rd = opbits( 25, 5 );
            uint32_t op3 = opbits( 19, 6 );
            uint32_t rs1 = opbits( 14, 5 );
            uint32_t i = opbit( 13 );
            uint32_t rs2 = opbits( 0, 5 );                              // decode both rs2 and simm13 since the conditional is slower
            int32_t simm13 = sign_extend( opbits( 0, 13 ), 12 );

            if ( 2 == op ) // arithmetic, logical, shift, and remaining
            {
                switch( op3 )
                {
                    case 0: // add
                    case 8: // addx
                    case 0x10: // addcc
                    case 0x18: // addxcc
                    case 0x20: // taddcc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t c = ( 0x8 == op3 || 0x18 == op3 ) ? flag_c() : 0;
                        uint64_t result64 = (uint64_t) a + (uint64_t) b + (uint64_t) c;
                        uint32_t result32 = 0xffffffff & result64;
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result32;
                        if ( 0x10 == op3 || 0x18 == op3 || 0x20 == op3 )
                        {
                            set_zn( result32 );
                            setflag_c( 0 != ( 0xffffffff00000000 & result64 ) );
                            bool signa = sign32( a );
                            bool signb = sign32( b );
                            bool signresult = sign32( result32 );
                            bool overflow = ( ( signa == signb ) && ( signa != signresult ) );
                            if ( ( 0x20 == op3 ) && ( ( a & 3 ) || ( b & 3 ) ) )
                                overflow = true;
                            setflag_v( overflow );

                        }
                        break;
                    }
                    case 1: // and
                    case 0x11: // andcc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t result = a & b;
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result;
                        if ( 0x11 == op3 )
                            set_zn( result );
                        break;
                    }
                    case 2: // or
                    case 0x12: // orcc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t result = a | b;
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result;
                        if ( 0x12 == op3 )
                            set_zn( result );
                        break;
                    }
                    case 3: // xor
                    case 0x13: // xorcc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t result = a ^ b;
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result;
                        if ( 0x13 == op3 )
                            set_zn( result );
                        break;
                    }
                    case 4: // sub
                    case 0xc: // subx
                    case 0x14: // subcc
                    case 0x1c: // subxcc
                    case 0x21: // tsubcc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t c = ( 0xc == op3 || 0x1c == op3 ) ? flag_c() : 0;
                        uint32_t diff = a - b - c;
                        if ( 0 != rd )
                            Sparc_reg( rd ) = diff;
                        if ( 0x14 == op3 || 0x1c == op3 || 0x21 == op3 )
                        {
                            set_zn( diff );
                            bool signa = sign32( a );
                            bool signb = sign32( b );
                            bool signdiff = sign32( diff );
                            setflag_c( ( !signa && signb ) || ( signdiff && ( !signa || signb ) ) );
                            bool overflow = ( ( signa != signb ) && ( signdiff != signa ) );
                            if ( ( 0x21 == op3 ) && ( ( a & 3 ) || ( b & 3 ) ) )
                                overflow = true;
                            setflag_v( overflow );
                        }
                        break;
                    }
                    case 5: // andn
                    case 0x15: // andncc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t result = ( a & ~b );
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result;
                        if ( 0x15 == op3 )
                            set_zn( result );
                        break;
                    }
                    case 6: // orn
                    case 0x16: // orncc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t result = ( a | ~b );
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result;
                        if ( 0x15 == op3 )
                            set_zn( result );
                        break;
                    }
                    case 7: // xnor
                    case 0x17: // xnorcc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t result = ~ ( a ^ b );
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result;
                        if ( 0x17 == op3 )
                            set_zn( result );
                        break;
                    }
                    case 0xa: // umul
                    case 0x1a: // umulcc
                    {

                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint64_t result64 = (uint64_t) a * (uint64_t) b;
                        uint32_t result32 = (uint32_t) ( result64 & 0xffffffff );
                        y = (uint32_t) ( 0xffffffff & ( result64 >> 32 ) );
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result32;
                        if ( 0x1a == op3 )
                        {
                            set_zn( result32 );
                            setflag_v( false );
                            setflag_c( false );
                        }
                        break;
                    }
                    case 0xb: // smul
                    case 0x1b: // smulcc
                    {

                        int32_t a = Sparc_reg( rs1 );
                        int32_t b = i ? simm13 : Sparc_reg( rs2 );
                        int64_t result64 = (int64_t) a * (int64_t) b;
                        uint32_t result32 = (uint32_t) ( result64 & 0xffffffff );
                        y = (uint32_t) ( 0xffffffff & ( result64 >> 32 ) );
                        if ( 0 != rd )
                            Sparc_reg( rd ) = result32;
                        if ( 0x1b == op3 )
                        {
                            set_zn( result32 );
                            setflag_v( false );
                            setflag_c( false );
                        }
                        break;
                    }
                    case 0xe: // udiv
                    case 0x1e: // udivcc
                    {
                        uint32_t a = Sparc_reg( rs1 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint64_t dividend = ( ( (uint64_t) y ) << 32 ) | a;
                        if ( 0 != b ) // bugbug: trap here
                        {
                            uint64_t result64 = dividend / (uint64_t ) b;
                            bool overflow = ( result64 & 0xffffffff00000000 );
                            uint32_t result32 = overflow ? 0xffffffff : (uint32_t) result64;
                            if ( 0x1e == op3 )
                            {
                                setflag_c( false );
                                setflag_v( overflow );
                                set_zn( result32 );
                            }
                            if ( 0 != rd )
                                Sparc_reg( rd ) = result32;
                        }
                        break;
                    }
                    case 0xf: // sdiv
                    case 0x1f: // sdivcc
                    {
                        int32_t a = Sparc_reg( rs1 );
                        int32_t b = i ? simm13 : Sparc_reg( rs2 );
                        int64_t dividend = ( ( (uint64_t) y ) << 32 ) | a;
                        if ( 0 != b ) // bugbug: trap here
                        {
                            int64_t result64 = dividend / (int64_t ) b;
                            uint32_t hiresult = (uint32_t) ( result64 >> 32 );
                            bool overflow = ( 0xffffffff != hiresult && 0 != hiresult );
                            bool sign = sign32( hiresult );
                            uint32_t result32 = (uint32_t) result64;
                            if ( overflow )
                                result32 = sign ? 0x80000000 : 0x7fffffff;
                            if ( 0x1f == op3 )
                            {
                                setflag_c( false );
                                setflag_v( overflow );
                                set_zn( result32 );
                            }
                            if ( 0 != rd )
                                Sparc_reg( rd ) = result32;
                        }
                        break;
                    }
                    case 0x24: // mulscc.  Use gcc's -mcpu=v7 flag to generate usage of this instruction for integer multiplication (sparc v8 has native integer multiplication)
                    {
                        // op1 = (n XOR v) CONCAT r[rs1]<31:1>
                        // if (Y<0> = 0) op2 = 0, else op2 = r[rs2] or sign extnd(simm13)
                        // r[rd] <== op1 + op2
                        // Y <== r[rs1]<0> CONCAT Y<31:1>
                        // n <== r[rd]<31>
                        // z <== if [r[rd]]=0 then 1, else 0
                        // v <== ((op1<31> AND op2<31> AND not r[rd]<31>) OR (not op1<31> AND not op2<31> AND r[rd]<31>))
                        // c <== ((op1<31> AND op2<31>) OR (not r[rd] AND (op1<31> OR op2<31>))

                        uint32_t rs1val = Sparc_reg( rs1 );
                        uint32_t operand1 = ( rs1val >> 1 ) | ( ( flag_n() ^ flag_v() ) << 31 );
                        uint32_t b = i ? simm13 : Sparc_reg( rs2 );
                        uint32_t operand2 = ( y & 1 ) ? b : 0;
                        uint32_t rdval = operand1 + operand2;
                        Sparc_reg( rd ) = rdval;
                        y = ( y >> 1 ) | ( ( rs1val & 1 ) << 31 );
                        set_zn( rdval );
                        bool sign_op1 = sign32( operand1 );
                        bool sign_op2 = sign32( operand2 );
                        bool sign_rdval = sign32( rdval );
                        setflag_v( ( sign_op1 & sign_op2 & !sign_rdval ) | ( !sign_op1 & !sign_op2 & sign_rdval ) );
                        setflag_c( ( sign_op1 & sign_op2 ) | ( ( 0 == rdval ) & sign_op1 | sign_op2 ) );
                        break;
                    }
                    case 0x25: // sll
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = Sparc_reg( rs1 ) << ( 0x1f & ( i ? simm13 : Sparc_reg( rs2 ) ) );
                        break;
                    }
                    case 0x26: // srl
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = Sparc_reg( rs1 ) >> ( 0x1f & ( i ? simm13 : Sparc_reg( rs2 ) ) );
                        break;
                    }
                    case 0x27: // sra
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = ( (int32_t) Sparc_reg( rs1 ) ) >> ( 0x1f & ( i ? simm13 : Sparc_reg( rs2 ) ) );
                        break;
                    }
                    case 0x28: // rdy
                    {
                        if ( 0 == rs1 ) // rdy
                        {
                            if ( 0 != rd )
                                Sparc_reg( rd ) = y;
                        }
                        else if ( 0xf == rs1 ) // stbar
                            { /* do nothing */ }
                        else
                            unhandled();
                        break;
                    }
                    case 0x30: // wry, wrasr, wrpsr, wrwim, wrtbr
                    {
                        if ( 0 == rd ) // wry
                            y = Sparc_reg( rs1 ) ^ ( i ? simm13 : Sparc_reg( rs2 ) );
                        else
                            unhandled();
                        break;
                    }
                    case 0x34: // fmovs, fnegs, fabss, fmuls, fmuld, fmulq, fsmuld, fsmulq, fdivs, fdivd, fdivq
                    {
                        uint32_t opf = opbits( 5, 9 );
                        switch( opf )
                        {
                            case 1: fregs[ rd ] = fregs[ rs2 ]; break;                                                        // fmovs
                            case 5: fregs[ rd ] = -fregs[ rs2 ]; break;                                                       // fnegs
                            case 9: fregs[ rd ] = fabsf( fregs[ rs2 ] ); break;                                               // fabss
                            case 0x29: fregs[ rd ] = sqrtf( fregs[ rs2 ] ); break;                                            // fsqrts
                            case 0x2a: set_dreg( rd, sqrt( get_dreg( rs2 ) ) ); break;                                        // fsqrtd
                            case 0x2b: set_qreg( rd, sqrtl( get_qreg( rs2 ) ) ); break;                                       // fsqrtq
                            case 0x41: fregs[ rd ] = (float) do_fadd( fregs[ rs1 ], fregs[ rs2 ] ); break;                    // fadds
                            case 0x42: set_dreg( rd, do_fadd( get_dreg( rs1 ), get_dreg( rs2 ) ) ); break;                    // faddd
                            case 0x43: set_qreg( rd, get_qreg( rs1 ) + get_qreg( rs2 ) ); break;                              // faddq
                            case 0x45: fregs[ rd ] = (float) do_fsub( fregs[ rs1 ], fregs[ rs2 ] ); break;                    // fsubs
                            case 0x46: set_dreg( rd, do_fsub( get_dreg( rs1 ), get_dreg( rs2 ) ) ); break;                    // fsubd
                            case 0x47: set_qreg( rd, get_qreg( rs1 ) - get_qreg( rs2 ) ); break;                              // fsubq
                            case 0x49: fregs[ rd ] = (float) do_fmul( fregs[ rs1 ], fregs[ rs2 ] ); break;                    // fmuls
                            case 0x4a: set_dreg( rd, do_fmul( get_dreg( rs1 ), get_dreg( rs2 ) ) ); break;                    // fmuld
                            case 0x4b: set_qreg( rd, get_qreg( rs1 ) * get_qreg( rs2 ) ); break;                              // fmulq
                            case 0x4d: fregs[ rd ] = (float) do_fdiv( fregs[ rs1 ], fregs[ rs2 ] ); break;                    // fdivs
                            case 0x4e: set_dreg( rd, do_fdiv( get_dreg( rs1 ), get_dreg( rs2 ) ) ); break;                    // fdivd
                            case 0x4f: set_qreg( rd, get_qreg( rs1 ) / get_qreg( rs2 ) ); break;                              // fdivq
                            case 0x69: set_dreg( rd, do_fmul( fregs[ rs1 ], (double) fregs[ rs2 ] ) ); break;                 // fsmuld
                            case 0x6e: set_qreg( rd, (long double) get_dreg( rs1 ) * (long double) get_dreg( rs2 ) ); break;  // fsmulq
                            case 0xc4: fregs[ rd ] = (float) ( * (int32_t *) &fregs[ rs2 ] ); break;                          // fitos
                            case 0xc6: fregs[ rd ] = (float) get_dreg( rs2 ); break;                                          // fdtos
                            case 0xc7: fregs[ rd ] = (float) get_qreg( rs2 ); break;                                          // fqtos
                            case 0xc8: set_dreg( rd, (double) ( * (int32_t *) &fregs[ rs2 ] ) ); break;                       // fitod
                            case 0xc9: set_dreg( rd, fregs[ rs2 ] ); break;                                                   // fstod
                            case 0xcb: set_dreg( rd, (double) get_qreg( rs2 ) ); break;                                       // fqtod
                            case 0xcc: set_qreg( rd, (long double) ( * (int32_t *) &fregs[ rs2 ] ) ); break;                  // fitoq
                            case 0xcd: set_qreg( rd, fregs[ rs2 ] ); break;                                                   // fstoq
                            case 0xce: set_qreg( rd, get_dreg( rs2 ) ); break;                                                // fdtoq
                            case 0xd1: * (int32_t *) & fregs[ rd ] = (int32_t) truncf( fregs[ rs2 ] ); break;                 // fstoi  these 3 round towards 0 and ignore RD in FSR
                            case 0xd2: * (int32_t *) & fregs[ rd ] = (int32_t) trunc( get_dreg( rs2 ) ); break;               // fdtoi
                            case 0xd3: * (int32_t *) & fregs[ rd ] = (int32_t) truncl( get_qreg( rs2 ) ); break;              // fqtoi
                            default: unhandled(); break;
                        }

                        trace_fregs();
                        break;
                    }
                    case 0x35: // fcmps, fcpmd, fpmpq and exception variants
                    {
                        uint32_t opf = opbits( 5, 9 );
                        switch( opf )
                        {
                            case 0x51: set_fcc( compare_floating( fregs[ rs1 ], fregs[ rs2 ] ) ); break;       // fcmps
                            case 0x52: set_fcc( compare_floating( get_dreg( rs1 ), get_dreg( rs2 ) ) ); break; // fcmpd
                            case 0x53: set_fcc( compare_floating( get_qreg( rs1 ), get_qreg( rs2 ) ) ); break; // fcmpq
                            case 0x55: set_fcc( compare_floating( fregs[ rs1 ], fregs[ rs2 ] ) ); break;       // fcmpes  bugbug: no exceptions yet for these 3 fcmpeX variants
                            case 0x56: set_fcc( compare_floating( get_dreg( rs1 ), get_dreg( rs2 ) ) ); break; // fcmped
                            case 0x57: set_fcc( compare_floating( get_qreg( rs1 ), get_qreg( rs2 ) ) ); break; // fcmpeq
                            default: unhandled(); break;
                        }
                        trace_fregs();
                        break;
                    }
                    case 0x38: // jmpl
                    {
                        npc = Sparc_reg( rs1 ) + ( i ? simm13 : Sparc_reg( rs2 ) ); // jump to here after executing instruction in the delay slot
                        if ( 0 != rd )
                            Sparc_reg( rd ) = pc;
                        delay_instruction = 1;
                        pc += 4;
                        break;
                    }
                    case 0x3a: // tcc
                    {
                        uint32_t cond = opbits( 25, 4 );
                        if ( check_condition( cond ) )
                        {
                            uint32_t trap = 128 + ( 0x7f & Sparc_reg( rs1 ) );
                            trap +=  ( i ? simm13 : Sparc_reg( rs2 ) );
                            handle_trap( trap );
                        }
                        break;
                    }
                    case 0x3b: { break; } // flush
                    case 0x3c: // save
                    {
                        uint32_t cwp_new = next_save_cwp( get_cwp() );
                        if ( get_bit32( wim, cwp_new ) ) // generate overflow trap
                        {
                            //tracer.Trace( "  save generating overflow trap with delay_instruction %u\n", delay_instruction );
                            handle_trap( 5 );
                            if ( 2 == delay_instruction ) // if save was in a delay slot, remember that
                                delay_instruction = 1;
                            else
                                npc = pc;
                            break;
                        }
                        else
                        {
                            // math from old cwp and store the result in the new cwp
                            uint32_t result = Sparc_reg( rs1 ) + ( i ? simm13 : Sparc_reg( rs2 ) );
                            set_cwp( cwp_new );
                            if ( 0 != rd )
                                Sparc_reg( rd ) = result;
                        }
                        break;
                    }
                    case 0x3d: // restore
                    {
                        uint32_t cwp_new = next_restore_cwp( get_cwp() );
                        if ( get_bit32( wim, cwp_new ) ) // generate underflow trap
                        {
                            //tracer.Trace( "  restore generating underflow trap with delay_instruction %u\n", delay_instruction );
                            handle_trap( 6 );
                            if ( 2 == delay_instruction ) // if restore was in a delay slot, remember that
                                delay_instruction = 1;
                            else
                                npc = pc;
                            break;
                        }
                        else
                        {
                            // math from old cwp and store the result in the new cwp
                            uint32_t result = Sparc_reg( rs1 ) + ( i ? simm13 : Sparc_reg( rs2 ) );
                            set_cwp( cwp_new );
                            if ( 0 != rd )
                                Sparc_reg( rd ) = result;
                        }
                        break;
                    }
                    default: unhandled();
                }
            }
            else // 3 == op. memory
            {
                uint32_t address = Sparc_reg( rs1 ) + ( i ? simm13 : Sparc_reg( rs2 ) );
                switch( op3 )
                {
                    case 0: // ld
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = getui32( address );
                        break;
                    }
                    case 1: // ldub
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = getui8( address );
                        break;
                    }
                    case 2: // lduh
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = getui16( address );
                        break;
                    }
                    case 3: // ldd
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = getui32(  address );
                        Sparc_reg( rd + 1 ) = getui32( address + 4 );
                        break;
                    }
                    case 4: // st
                    {
                        setui32( address, Sparc_reg( rd ) );
                        break;
                    }
                    case 5: // stb
                    {
                        setui8( address, 0xff & Sparc_reg( rd ) );
                        break;
                    }
                    case 6: // sth
                    {
                        setui16( address, 0xffff & Sparc_reg( rd ) );
                        break;
                    }
                    case 7: // std
                    {
                        setui32( address, Sparc_reg( rd ) );
                        setui32( address + 4, Sparc_reg( rd + 1 ) );
                        break;
                    }
                    case 9: // ldsb
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = sign_extend( getui8( address ), 7 );
                        break;
                    }
                    case 0xa: // ldsh
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = sign_extend( getui16( address ), 15 );
                        break;
                    }
                    case 0xd: // ldstub
                    {
                        if ( 0 != rd )
                            Sparc_reg( rd ) = getui8( address );
                        setui8( address, 0xff );
                        break;
                    }
                    case 0xf: // swap
                    {
                        uint32_t val = getui32( address );
                        setui32( address, Sparc_reg( rd ) );
                        Sparc_reg( rd ) = val;
                        break;
                    }
                    case 0x20: // ldf
                    {
                        fregs[ rd ] = getfloat( address );
                        trace_fregs();
                        break;
                    }
                    case 0x21: // ldfsr
                    {
                        fsr = getui32( address );
                        break;
                    }
                    case 0x23: // lddf
                    {
                        fregs[ rd ] = getfloat( address );
                        fregs[ rd + 1 ] = getfloat( address + 4 );
                        trace_fregs();
                        break;
                    }
                    case 0x24: // stf
                    {
                        setfloat( address, fregs[ rd ] );
                        break;
                    }
                    case 0x25: // stfsr
                    {
                        setui32( address, fsr );
                        break;
                    }
                    case 0x27: // stdf
                    {
                        setfloat( address, fregs[ rd ] );
                        setfloat( address + 4, fregs[ rd + 1 ] );
                        break;
                    }
                    default:
                        unhandled();
                }
            }
        }
    } // for

    return cycles; // for now, instructions not cycles
} //run

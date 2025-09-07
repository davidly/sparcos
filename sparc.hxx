#include <djl_os.hxx>

struct Sparc;

// callbacks from the cpu to the emulator

extern void emulator_invoke_svc( Sparc & cpu );                                                // called when the linux-style syscall instruction is executed
extern void emulator_invoke_sparc_trap5( Sparc & cpu );                                        // called for trap #5 overflow of register window from save
extern void emulator_invoke_sparc_trap6( Sparc & cpu );                                        // called for trap #6 underflow of register window from restore
extern const char * emulator_symbol_lookup( uint32_t address, uint32_t & offset );             // returns the best guess for a symbol name and offset for the address
extern void emulator_hard_termination( Sparc & cpu, const char *pcerr, uint64_t error_value ); // show an error and exit

struct Sparc
{
    bool trace_instructions( bool trace );         // enable/disable tracing each instruction
    void end_emulation( void );                    // make the emulator return at the start of the next instruction
    uint64_t run( void );                          // execute the instruction stream
    void trace_fregs();                            // debug trace the contents of the floating point registers

    Sparc( vector<uint8_t> & memory, uint32_t base_address, uint32_t start, uint32_t stack_commit, uint32_t top_of_stack )
    {
        reset( memory, base_address, start, stack_commit, top_of_stack );
    } //Sparc

    void reset( vector<uint8_t> & memory, uint32_t base_address, uint32_t start, uint32_t stack_commit, uint32_t top_of_stack )
    {
        memset( this, 0, sizeof( *this ) );
        wim = 0xffffffff;                          // set to this on reset and the OS owns its value
        stack_size = stack_commit;                 // remember how much of the top of RAM is allocated to the stack
        stack_top = top_of_stack;                  // where the stack started
        base = base_address;                       // lowest valid address in the app's address space, maps to offset 0 in mem. If not 0, can't update trap vectors.
        mem = memory.data();                       // save the pointer, but don't take ownership
        mem_size = (uint32_t) memory.size();       // how much RAM is allocated for the cpu
        beyond_mem = mem + memory.size();          // addresses beyond and later are illegal
        membase = mem - base;                      // real pointer to the start of the app's memory (prior to offset)

        if ( 0 != start )
        {
            npc = pc = start;                      // app start address
            //               impl            version       enable FP     enable traps current window pointer is 0
            psr = (uint32_t) ( ( 0xd << 28 ) | ( 1 << 24 ) | ( 1 << 12 ) | ( 1 << 5 ) | 0 ); // reasonable defaults
            Sparc_reg( 14 ) = top_of_stack;
        }
    } //Sparc

    inline uint32_t getui32( uint32_t o )
    {
        #ifdef TARGET_BIG_ENDIAN
            return * (uint32_t *) getmem( o );
        #else
            return flip_endian32( * (uint32_t *) getmem( o ) );
        #endif
    } //getui32

    inline uint16_t getui16( uint32_t o )
    {
        #ifdef TARGET_BIG_ENDIAN
            return * (uint16_t *) getmem( o );
        #else
            return flip_endian16( * (uint16_t *) getmem( o ) );
        #endif
    } //getui16

    inline uint8_t getui8( uint32_t o ) { return * (uint8_t *) getmem( o ); }

    inline void setui32( uint32_t o, uint32_t val )
    {
        #ifdef TARGET_BIG_ENDIAN
            * (uint32_t *) getmem( o ) = val;
        #else
            * (uint32_t *) getmem( o ) = flip_endian32( val );
        #endif
    } //setui32

    inline void setui16( uint32_t o, uint16_t val )
    {
        #ifdef TARGET_BIG_ENDIAN
            * (uint16_t *) getmem( o ) = val;
        #else
            * (uint16_t *) getmem( o ) = flip_endian16( val );
        #endif
    } //setui16

    inline void setui8( uint32_t o, uint8_t val ) { * (uint8_t *) getmem( o ) = val; }

    inline uint32_t get_vm_address( uint32_t offset )
    {
        return base + offset;
    } //get_vm_address

    inline uint8_t * getmem( uint32_t offset )
    {
        #ifdef NDEBUG
            return membase + offset;
        #else
            uint8_t * r = membase + offset;

            if ( r >= beyond_mem )
                emulator_hard_termination( *this, "memory reference beyond address space:", offset );

            if ( r < mem )
                emulator_hard_termination( *this, "memory reference prior to address space:", offset );

            return r;
        #endif
    } //getmem

    uint32_t host_to_vm_address( void * p )
    {
        return (uint32_t) ( (uint8_t *) p - mem + base );
    } //host_to_vm_address

    bool is_address_valid( uint32_t offset )
    {
        uint8_t * r = membase + offset;
        return ( ( r < beyond_mem ) && ( r >= mem ) );
    } //is_address_valid

    inline uint32_t getoffset( uint32_t address )
    {
        return address - base;
    } //getoffset

    uint32_t next_save_cwp( uint32_t w ) { return ( 0 == w ) ? ( NWINDOWS - 1 ) : ( w - 1 ); }
    uint32_t next_restore_cwp( uint32_t w ) { return ( ( NWINDOWS - 1 ) == w ) ? 0 : ( w + 1 ); }

    inline uint32_t get_cwp()
    {
        return ( psr & 0x1f );
    } //get_cwp

    inline void set_cwp( uint32_t x )
    {
        assert( x < 32 );
        psr &= ~ 0x1f;
        psr |= x;
    } //set_cwp

    uint32_t gregs[ 8 ];                           // global registers g0 - g7
    const static uint32_t NWINDOWS = 8;            // must be a power of 2 (4, 8, 16, or 32). 8 is typical for Sparc. 4 for test coverage. 32 for performance.
    uint32_t regs[ NWINDOWS * 16 ];                // 16 32-bit registers per window
    uint32_t psr;                                  // processor state register. includes 5 bits for cwp
    uint32_t wim;                                  // window invalid mask
    uint32_t tbr;                                  // trap base register
    uint32_t y;                                    // multiply/divide register
    uint32_t pc;                                   // program counter
    uint32_t npc;                                  // next program counter

    float fregs[ 32 ];                             // 32 32-bit floating point registers
    uint32_t fsr;                                  // floating-point state register

    uint32_t & Sparc_psr() { return psr; }
    uint32_t & Sparc_wim() { return wim; }
    uint32_t & Sparc_tbr() { return tbr; }

    uint32_t & Sparc_reg( uint32_t r )
    {
        assert ( r < 32 ); // 8 each of global, output, local, and input
        if ( r < 8 ) // global. use of g0 is very common
            return gregs[ r ];

        uint32_t cwp = get_cwp();
        uint32_t i = 0;
        if ( r < 16 ) // output. below cwp
        {
            if ( cwp > 0 )
                i = ( ( cwp - 1 ) * 16 ) + r;
            else
                i = ( ( NWINDOWS - 1 ) * 16 ) + r;
        }
        else // local or in for cwp
            i = ( cwp * 16 ) + r - 16;

        return regs[ i ];
    } //Sparc_reg

    inline void setflag_c( bool f ) { psr &= ~( 1 << 20 ); psr |= ( ( 0 != f ) << 20 ); }
    inline void setflag_v( bool f ) { psr &= ~( 1 << 21 ); psr |= ( ( 0 != f ) << 21 ); }
    inline void setflag_z( bool f ) { psr &= ~( 1 << 22 ); psr |= ( ( 0 != f ) << 22 ); }
    inline void setflag_n( bool f ) { psr &= ~( 1 << 23 ); psr |= ( ( 0 != f ) << 23 ); }

private:
    uint32_t opcode;                               // the currently executing opcode found at pc
    uint8_t * mem;                                 // RAM for the cpu is here
    uint8_t * beyond_mem;                          // first byte beyond the RAM
    uint32_t base;                                 // base address of the process per the elf file
    uint8_t * membase;                             // host pointer to base of vm's memory (mem - base) to make getmem() faster
    uint32_t stack_size;
    uint32_t stack_top;
    uint32_t mem_size;
    uint64_t cycles_so_far;                        // just an instruction count for now

    inline uint32_t opbits( uint32_t lowbit, uint32_t len ) const
    {
        uint32_t val = ( opcode >> lowbit );
        assert( 16 != len ); // the next line of code wouldn't work but there are no callers that do this
        return ( val & ( ( 1 << len ) - 1 ) );
    } //opbits

    inline uint32_t opbit( uint32_t bit ) const { return ( 1 & ( opcode >> bit ) ); }

    inline static int32_t sign_extend( uint32_t x, uint32_t high_bit )
    {
        assert( high_bit < 31 );
        x &= ( 1 << ( high_bit + 1 ) ) - 1; // clear bits above the high bit since they may contain noise
        const int32_t m = ( (uint32_t) 1 ) << high_bit;
        return ( x ^ m ) - m;
    } //sign_extend

    inline static int16_t sign_extend16( uint16_t x, uint16_t high_bit )
    {
        assert( high_bit < 15 );
        x &= ( 1 << ( high_bit + 1 ) ) - 1; // clear bits above the high bit since they may contain noise
        const int16_t m = ( (uint16_t) 1 ) << high_bit;
        return ( x ^ m ) - m;
    } //sign_extend16

    inline bool flag_c() { return ( 0 != ( psr & ( 1 << 20 ) ) ); }
    inline bool flag_v() { return ( 0 != ( psr & ( 1 << 21 ) ) ); }
    inline bool flag_z() { return ( 0 != ( psr & ( 1 << 22 ) ) ); }
    inline bool flag_n() { return ( 0 != ( psr & ( 1 << 23 ) ) ); }

    inline void set_zn( uint32_t x )
    {
        setflag_z( 0 == x );
        setflag_n( 0 != ( 0x80000000 & x ) );
    } //set_zn

    static inline uint32_t get_bit32( uint32_t x, uint32_t bit_number )
    {
        assert( bit_number < 32 );
        return ( ( x >> bit_number ) & 1 );
    } //get_bit32

    static inline uint32_t get_bits32( uint32_t x, uint32_t lowbit, uint32_t len )
    {
        uint32_t val = ( x >> lowbit );
        assert( 32 != len ); // the next line of code wouldn't work but there are no callers that do this
        return ( val & ( ( 1 << len ) - 1 ) );
    } //get_bits32

    inline float getfloat( uint32_t o )
    {
        uint32_t u = getui32( o );
        return * (float *) &u;
    } //getfloat

    inline void setfloat( uint32_t o, float fval ) { setui32( o, * (uint32_t *) &fval ); }

    static inline uint64_t flip_dwords( uint64_t x )
    {
        return ( x >> 32 ) | ( x << 32 );
    } //flip_dwords

    inline void set_dreg( uint32_t fr, double d )
    {
        #ifdef TARGET_BIG_ENDIAN
            * (double *) ( & fregs[ fr ] ) = d;
        #else
            * (uint64_t *) ( & fregs[ fr ] ) = flip_dwords( * (uint64_t *) &d );
        #endif
    } //set_dreg

    inline double get_dreg( uint32_t fr )
    {
        #ifdef TARGET_BIG_ENDIAN
            return * (double *) ( & fregs[ fr ] );
        #else
            uint64_t d = flip_dwords( * (uint64_t *) ( & fregs[ fr ] ) );
            return * (double *) &d;
        #endif
    } //get_dreg

    inline void set_qreg( uint32_t fr, long double d )
    {
        * (long double *) ( & fregs[ fr ] ) = d;
    } //set_dreg

    inline long double get_qreg( uint32_t fr )
    {
        return * (long double *) ( & fregs[ fr ] );
    } //get_qreg

    inline uint32_t get_fcc() { return get_bits32( fsr, 10, 2 ); }
    inline uint32_t get_rounding_mode() { return get_bits32( fsr, 30, 2 ); }

    inline void set_fcc( uint32_t fcc )
    {
        fsr &= ~ ( 3 << 10 );
        fsr |= ( fcc << 10 );
    } //set_fcc

    void handle_trap( uint32_t trap );
    void trace_state( void );                  // trace the machine current status
    void unhandled( void );
    const char * render_flags( void );
    const char render_fflag( void );
    bool check_condition( uint32_t cond );
    bool check_fcondition( uint32_t cond );
    void trace_canonical( const char * pins );
    void trace_shift_canonical( const char * pins );
    void trace_ld_canonical( const char * pins );
    void trace_st_canonical( const char * pins );
};


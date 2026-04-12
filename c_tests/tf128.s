! tf128_arch_correct.s
! GNU as syntax, Linux/SPARC V8, standalone _start
!
! Covered instructions:
!   fqtoi, fdtoq, fstoq, fitoq, fqtod, fqtos,
!   fdmulq, fdivq, fmulq, fsubq, faddq, fsqrtq,
!   fcmpq, fcmpeq
!
! Result:
!   success -> writes "completed with great success\n", exits 0
!   failure -> writes "failure\n", exits with failing test number
!
! Architectural assumptions:
!   - SPARC target format is big-endian
!   - binary128 constants are laid out in canonical big-endian word order
!   - a quad occupies %fN,%fN+1,%fN+2,%fN+3
!   - no swapped-pair workaround is assumed anywhere in this file
!
! Linux/SPARC syscall convention used here:
!   %g1 = syscall number, %o0..%o2 = args, ta 0x10

        .section ".text"
        .align 4
        .global _start
        .type   _start, #function

_start:
        call    run_fp128_tests
        nop

        mov     %o0, %g2                  ! preserve exit status

        cmp     %g2, 0
        bne     .Lfail_msg
        nop

        mov     4, %g1                    ! SYS_write
        mov     1, %o0                    ! stdout
        set     msg_success, %o1
        mov     msg_success_end - msg_success, %o2
        ta      0x10
        nop
        ba      .Lexit
        nop

.Lfail_msg:
        mov     4, %g1                    ! SYS_write
        mov     1, %o0
        set     msg_failure, %o1
        mov     msg_failure_end - msg_failure, %o2
        ta      0x10
        nop

.Lexit:
        mov     1, %g1                    ! SYS_exit
        mov     %g2, %o0
        ta      0x10
        nop

! ----------------------------------------------------------------------
! int run_fp128_tests(void)
!   returns %o0 = 0 on success, else failure code
! ----------------------------------------------------------------------

        .align 4
        .global run_fp128_tests
        .type   run_fp128_tests, #function

run_fp128_tests:
        save    %sp, -192, %sp

! stack locals:
!   [%fp-16]   temp int32
!   [%fp-24]   temp double (8 bytes)
!   [%fp-28]   temp single (4 bytes)

! Failure codes:
!   1   fitoq   3 -> 3.0q
!   2   fqtoi   3.0q -> 3
!   3   fdtoq   1.5d -> 1.5q
!   4   fstoq   2.0f -> 2.0q
!   5   fqtod   1.5q -> 1.5d
!   6   fqtos   2.0q -> 2.0f
!   7   fdmulq  2.0d * 0.5d -> 1.0q
!   8   faddq   1.0q + 2.0q -> 3.0q
!   9   fsubq   3.0q - 1.0q -> 2.0q
!   10  fmulq   3.0q * 0.5q -> 1.5q
!   11  fdivq   3.0q / 2.0q -> 1.5q
!   12  fsqrtq  4.0q -> 2.0q
!   13  fcmpq   1.0q < 2.0q
!   14  fcmpq   2.0q == 2.0q
!   15  fcmpq   qNaN unordered 1.0q
!   16  fcmpeq  2.0q == 2.0q
!   17  fcmpeq  1.0q < 2.0q

! ------------------------------------------------------------
! Test 1: fitoq 3 -> 3.0q
! ------------------------------------------------------------
        mov     3, %l0
        st      %l0, [%fp-16]
        ld      [%fp-16], %f0
        fitoq   %f0, %f4

        set     q_three, %l1
        ldd     [%l1],   %f8
        ldd     [%l1+8], %f10

        fcmpq   %f4, %f8
        nop
        nop
        nop
        fbe     .Ltest2
        nop
        mov     1, %i0
        ret
        restore

.Ltest2:
! ------------------------------------------------------------
! Test 2: fqtoi 3.0q -> 3
! ------------------------------------------------------------
        set     q_three, %l0
        ldd     [%l0],   %f4
        ldd     [%l0+8], %f6

        fqtoi   %f4, %f0
        st      %f0, [%fp-16]
        ld      [%fp-16], %l1
        cmp     %l1, 3
        be      .Ltest3
        nop
        mov     2, %i0
        ret
        restore

.Ltest3:
! ------------------------------------------------------------
! Test 3: fdtoq 1.5d -> 1.5q
! ------------------------------------------------------------
        set     d_one_point_five, %l0
        ldd     [%l0], %f0
        fdtoq   %f0, %f4

        set     q_one_point_five, %l1
        ldd     [%l1],   %f8
        ldd     [%l1+8], %f10

        fcmpq   %f4, %f8
        nop
        nop
        nop
        fbe     .Ltest4
        nop
        mov     3, %i0
        ret
        restore

.Ltest4:
! ------------------------------------------------------------
! Test 4: fstoq 2.0f -> 2.0q
! ------------------------------------------------------------
        set     s_two, %l0
        ld      [%l0], %f0
        fstoq   %f0, %f4

        set     q_two, %l1
        ldd     [%l1],   %f8
        ldd     [%l1+8], %f10

        fcmpq   %f4, %f8
        nop
        nop
        nop
        fbe     .Ltest5
        nop
        mov     4, %i0
        ret
        restore

.Ltest5:
! ------------------------------------------------------------
! Test 5: fqtod 1.5q -> 1.5d
! ------------------------------------------------------------
        set     q_one_point_five, %l0
        ldd     [%l0],   %f4
        ldd     [%l0+8], %f6

        fqtod   %f4, %f0
        std     %f0, [%fp-24]

        set     d_one_point_five, %l1
        ld      [%fp-24], %l2
        ld      [%l1],    %l3
        cmp     %l2, %l3
        bne     .Lf5_fail
        nop
        ld      [%fp-20], %l2
        ld      [%l1+4],  %l3
        cmp     %l2, %l3
        be      .Ltest6
        nop
.Lf5_fail:
        mov     5, %i0
        ret
        restore

.Ltest6:
! ------------------------------------------------------------
! Test 6: fqtos 2.0q -> 2.0f
! ------------------------------------------------------------
        set     q_two, %l0
        ldd     [%l0],   %f4
        ldd     [%l0+8], %f6

        fqtos   %f4, %f0
        st      %f0, [%fp-28]

        ld      [%fp-28], %l2
        set     s_two, %l1
        ld      [%l1], %l3
        cmp     %l2, %l3
        be      .Ltest7
        nop
        mov     6, %i0
        ret
        restore

.Ltest7:
! ------------------------------------------------------------
! Test 7: fdmulq 2.0d * 0.5d -> 1.0q
! ------------------------------------------------------------
        set     d_two, %l0
        ldd     [%l0], %f0
        set     d_half, %l1
        ldd     [%l1], %f2

        fdmulq  %f0, %f2, %f4

        set     q_one, %l2
        ldd     [%l2],   %f8
        ldd     [%l2+8], %f10

        fcmpq   %f4, %f8
        nop
        nop
        nop
        fbe     .Ltest8
        nop
        mov     7, %i0
        ret
        restore

.Ltest8:
! ------------------------------------------------------------
! Test 8: faddq 1.0q + 2.0q -> 3.0q
! ------------------------------------------------------------
        set     q_one, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_two, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        faddq   %f0, %f4, %f8

        set     q_three, %l2
        ldd     [%l2],   %f12
        ldd     [%l2+8], %f14

        fcmpq   %f8, %f12
        nop
        nop
        nop
        fbe     .Ltest9
        nop
        mov     8, %i0
        ret
        restore

.Ltest9:
! ------------------------------------------------------------
! Test 9: fsubq 3.0q - 1.0q -> 2.0q
! ------------------------------------------------------------
        set     q_three, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_one, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        fsubq   %f0, %f4, %f8

        set     q_two, %l2
        ldd     [%l2],   %f12
        ldd     [%l2+8], %f14

        fcmpq   %f8, %f12
        nop
        nop
        nop
        fbe     .Ltest10
        nop
        mov     9, %i0
        ret
        restore

.Ltest10:
! ------------------------------------------------------------
! Test 10: fmulq 3.0q * 0.5q -> 1.5q
! ------------------------------------------------------------
        set     q_three, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_half, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        fmulq   %f0, %f4, %f8

        set     q_one_point_five, %l2
        ldd     [%l2],   %f12
        ldd     [%l2+8], %f14

        fcmpq   %f8, %f12
        nop
        nop
        nop
        fbe     .Ltest11
        nop
        mov     10, %i0
        ret
        restore

.Ltest11:
! ------------------------------------------------------------
! Test 11: fdivq 3.0q / 2.0q -> 1.5q
! ------------------------------------------------------------
        set     q_three, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_two, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        fdivq   %f0, %f4, %f8

        set     q_one_point_five, %l2
        ldd     [%l2],   %f12
        ldd     [%l2+8], %f14

        fcmpq   %f8, %f12
        nop
        nop
        nop
        fbe     .Ltest12
        nop
        mov     11, %i0
        ret
        restore

.Ltest12:
! ------------------------------------------------------------
! Test 12: fsqrtq 4.0q -> 2.0q
! ------------------------------------------------------------
        set     q_four, %l0
        ldd     [%l0],   %f4
        ldd     [%l0+8], %f6

        fsqrtq  %f4, %f8

        set     q_two, %l1
        ldd     [%l1],   %f12
        ldd     [%l1+8], %f14

        fcmpq   %f8, %f12
        nop
        nop
        nop
        fbe     .Ltest13
        nop
        mov     12, %i0
        ret
        restore

.Ltest13:
! ------------------------------------------------------------
! Test 13: fcmpq 1.0q < 2.0q
! ------------------------------------------------------------
        set     q_one, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_two, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        fcmpq   %f0, %f4
        nop
        nop
        nop
        fbl     .Ltest14
        nop
        mov     13, %i0
        ret
        restore

.Ltest14:
! ------------------------------------------------------------
! Test 14: fcmpq 2.0q == 2.0q
! ------------------------------------------------------------
        set     q_two, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_two, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        fcmpq   %f0, %f4
        nop
        nop
        nop
        fbe     .Ltest15
        nop
        mov     14, %i0
        ret
        restore

.Ltest15:
! ------------------------------------------------------------
! Test 15: fcmpq qNaN unordered with 1.0q
! ------------------------------------------------------------
        set     q_qnan, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_one, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        fcmpq   %f0, %f4
        nop
        nop
        nop
        fbu     .Ltest16
        nop
        mov     15, %i0
        ret
        restore

.Ltest16:
! ------------------------------------------------------------
! Test 16: fcmpeq 2.0q == 2.0q
! ------------------------------------------------------------
        set     q_two, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_two, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        fcmpeq  %f0, %f4
        nop
        nop
        nop
        fbe     .Ltest17
        nop
        mov     16, %i0
        ret
        restore

.Ltest17:
! ------------------------------------------------------------
! Test 17: fcmpeq 1.0q < 2.0q
! ------------------------------------------------------------
        set     q_one, %l0
        ldd     [%l0],   %f0
        ldd     [%l0+8], %f2

        set     q_two, %l1
        ldd     [%l1],   %f4
        ldd     [%l1+8], %f6

        fcmpeq  %f0, %f4
        nop
        nop
        nop
        fbl     .Ldone
        nop
        mov     17, %i0
        ret
        restore

.Ldone:
        clr     %i0
        ret
        restore

        .section ".rodata"
        .align 16

msg_success:
        .ascii  "tf128 completed with great success\n"
msg_success_end:

msg_failure:
        .ascii  "tf128 failure (only sparcos built with g++ on x86/amd64/arm64 should work)\n"
msg_failure_end:

        .align 8

! single precision constants
s_two:
        .word   0x40000000              ! 2.0f

        .align 8

! double precision constants
d_half:
        .word   0x3fe00000, 0x00000000  ! 0.5
d_one_point_five:
        .word   0x3ff80000, 0x00000000  ! 1.5
d_two:
        .word   0x40000000, 0x00000000  ! 2.0

        .align 16

! canonical big-endian IEEE-754 binary128 constants
q_half:
        .word   0x3ffe0000, 0x00000000, 0x00000000, 0x00000000

q_one:
        .word   0x3fff0000, 0x00000000, 0x00000000, 0x00000000

q_one_point_five:
        .word   0x3fff8000, 0x00000000, 0x00000000, 0x00000000

q_two:
        .word   0x40000000, 0x00000000, 0x00000000, 0x00000000

q_three:
        .word   0x40008000, 0x00000000, 0x00000000, 0x00000000

q_four:
        .word   0x40010000, 0x00000000, 0x00000000, 0x00000000

q_qnan:
        .word   0x7fff8000, 0x00000000, 0x00000000, 0x00000000

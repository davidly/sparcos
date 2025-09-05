! compute the first 192 digits of e
! replicates this C code:
!    #define DIGITS_TO_FIND 200 /*9009*/
!    int main() {
!      int N = DIGITS_TO_FIND;
!      int x = 0;
!      int a[ DIGITS_TO_FIND ];
!      int n;
!
!      for (n = N - 1; n > 0; --n)
!          a[n] = 1;
!
!      a[1] = 2, a[0] = 0;
!      while (N > 9) {
!          n = N--;
!          while (--n) {
!              a[n] = x % n;
!              x = 10 * a[n-1] + x/n;
!          }
!          printf("%d", x);
!      }
!
!      printf( "\ndone\n" );
!      return 0;
!    }
!
! array is in %l2
! & a[n] is in %l1
! n is in %l5
! x is in %l4
! N is in %l3
   
    .section	".text"
	.align 8
.done_string:
	.asciz	"\ndone\n"
.number_string:
	.asciz	"%u"
.equ array_size, 200
	.align 8
.array:
    .zero 2*array_size    

	.section .text.startup,"ax",@progbits
	.align 4
	.global main
	.type	main, #function
	.proc	04
main:
	.cfi_startproc
	save	%sp, -96, %sp
	.cfi_window_save
	.cfi_register 15, 31
	.cfi_def_cfa_register 30

    sethi 64, %l0
    add %l0, 1, %l0
    mov array_size * 2, %l1
    sub %l1, 4, %l1
    sethi %hi(.array), %l2
    add %l2, %lo(.array), %l2
  .init_next:
    st %l0, [%l2 + %l1]
    subcc %l1, 4, %l1
    bne,a .init_next
    nop
    
    sth %g0, [%l2]
    mov 2, %l0
    sth %l0, [%l2 + 2]

    mov 10, %l6
    mov array_size, %l3
    clr %l4
    wr 0, %y

  outer:
    cmp %l3, 9             ! while ( N > 9 )
    beq loop_done
    sub %l3, 1, %l3        ! N = N - 1.  in the beq delay slot, always executed
    mov %l3, %l5           ! n = N
    mov %l2, %l1           ! move array pointer to %l1
    add %l1, %l5, %l1      ! add n to the pointer twice because each element is 2 bytes
    add %l1, %l5, %l1

  inner:
    udiv %l4, %l5, %o0     ! x / n
    umul %o0, %l5, %l0     ! begin to compute the remainder
    sub %l4, %l0, %l0      ! set %l0 to x % n, which is ( n - ( ( x / n ) * n ) )
    sth %l0, [%l1]         ! a[n] = x % n
    sub %l1, 2, %l1        ! move the current array pointer down one element
    lduh [%l1], %l0        ! load a[ n - 1 ] to %l0
    umul %l0, 10, %l0
    subcc %l5, 1, %l5      ! while ( --n )
    bne inner
    add %l0, %o0, %l4      ! x = 10 * a[ n - 1 ] + x / n. in the bne delay slot, always executed
 
  print_digit:
	sethi %hi(.number_string), %o0
	add %o0, %lo(.number_string), %o0	
    mov %l4, %o1
	call	printf, 0
 	nop                    ! delay slot
    ba,a outer
    
  loop_done:
	sethi %hi(.done_string), %o0
	add %o0, %lo(.done_string), %o0	
	call	puts, 0
	nop

	mov	0, %i0
	jmp	%i7+8
    restore
	.cfi_endproc
.LFE967:
	.size	main, .-main
	.section	.note.GNU-stack,"",@progbits

 .PAGE 'A test program for A65'
 .opt list
;******************************************
; Test file for the 6502 assembler - A65
; assemble as
;     A65 TEST.S
;******************************************
;; comment treatment
;******************************************
AA = $10; ';' immediately after the '0'
B = $20 space to comment subfield
C = $30	tab to comment subfield
DEFGHIjkl = $FFEE
D =DEFGHIjkl
;******************************************
; Number formats
;******************************************
 .byte %0101 ; binary number
 .byte @22 ; octal number
 .byte 22 ; decimal number
 .byte $22,$ff,$FF ; hex - upper/lower case
 .byte 'a','b' ; single ASCII characters
 lda #'x ; single ASCII character
 lda #''' ; single ASCII character - quote
;******************************************
;; ASCII character strings
;******************************************
 .byte 'abcd',0
 .byte 'Jim''s bicycle'
;******************************************
; Operation checks
;******************************************
 .word aa+B ; addition
 .word aa-B ; subtraction
 .word aa*B ; multiplication
 .word B/aa ; division
 .word C%B ; modulo
 .word B^C ; exclusive OR
 .word ~C ; one's complement
 .word B&C ; logical AND
 .word aa|B ; logical OR
 .word <D ; low byte
 .word >D ; high byte
 .word * ; current location
 .word aa,B,C
 .word B*[aa+C] ; one level of parenthesis
 .dbyte D ; high byte-low byte word
 .word D/256,D%256
;******************************************
; Addressing Mode Check
;******************************************
 *=$0100
 lda #aa ; immediate addressing
 lda D ; direct addessing
 LDA aa ; page zero addressing, aa < 256
a1 = 512
a2 = 500
 lda a1-a2 ; also page zero
 asl A ; accumulator addressing
 AsL a ; accumulator addressing also
 brk ; implied addressing
 lda (aa,X) ; indirect,X addressing
 lda (aa),Y ; indirect,Y addressing
 lda aa,X ; zero page,X addressing
 lda D,X ; absolute,X addressing
 lda D,Y ; absolute,Y addressing
 bcc *-$10 ; relative addressing
 jmp (D) ; indirect addressing
 ldx aa,Y ; zero page,Y addressing
 ldx aa,y ; alternate index name
 .opt nol
 ; if this comes out NOLIST doesnt work!   ****
 .opt list
;******************************************
; opcode check
;******************************************
 adc #01
 and #01
 asl A
 bcc *+2
 bcs *+2
 beq *+2
 bit $01
 bmi *+2
 bne *+2
 bpl *+2
 brk
 bvc *+2
 bvs *+2
 clc
 cld
 cli
 clv
 cmp #01
 cpx #01
 cpy #01
 dec $01
 dex
 dey
 eor #01
 inc $01
 inx
 iny
 jmp *+3
 jsr *+3
 lda #01
 ldx #01
 ldy #01
 lsr A
 nop
 ora #01
 pha
 php
 pla
 plp
 rol A
 ror A
 rti
 rts
 sbc #01
 sec
 sed
 sei
 sta $01
 stx $01
 sty $01
 tax
 tay
 tsx
 txa
 txs
 tya

 .skip ; not implemented (should generate a warning)
 .opt NOL
; if this comes out NOLIST doesnt work!
 .opt LIST
 .opt ERRORS, NOERRORS, GENERATE, NOGENERATE, SYM, NOSYM

finis .end ; end of assembly (optional)

 nop ; if this comes out .END doesnt work!


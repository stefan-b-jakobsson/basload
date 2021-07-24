;BSD 2-Clause License
;
;Copyright (c) 2021, Stefan Jakobsson
;All rights reserved.

;Redistribution and use in source and binary forms, with or without
;modification, are permitted provided that the following conditions are met:
;
;1. Redistributions of source code must retain the above copyright notice, this
;   list of conditions and the following disclaimer.
;
;2. Redistributions in binary form must reproduce the above copyright notice,
;   this list of conditions and the following disclaimer in the documentation
;   and/or other materials provided with the distribution.
;
;THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
;DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
;FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
;SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
;CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
;OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.ZEROPAGE
    util_ptr: .res 2
.CODE

;******************************************************************************
;Function name.......: util_bin_to_bcd
;Purpose.............: Converts a binary value to a BCD value
;Input...............: 24 bit binary value, X=low, Y=mid, A=high
;Returns.............: Pointer to 32 bit BCD value, X=addressL, Y=addressH
;Errors..............: None
;Credits.............: This routine is based on Andrew Jacobs code published
;                      here: http://www.6502.org/source/integers/hex2dec-more.htm
;                      The function basically switches to decimal mode and then
;                      adds the bits together
.proc util_bin_to_bcd
    ;Store 24 bit input
    stx input
    sty input+1
    sta input+2

    ;Clear 32 bit output
    stz output
    stz output+1
    stz output+2
    stz output+3

    ;Number of input bits
    ldx #24

    ;Set decimal mode
    sed

loop:
    ;Rotate input, leftmost bit -> C
    asl input
    rol input+1
    rol input+2
    
    ;32 bit addition. Value of C from previous operation is the actual input. Therefore C is not cleared.
    lda output
    adc output
    sta output
    
    lda output+1
    adc output+1
    sta output+1

    lda output+2
    adc output+2
    sta output+2

    lda output+3
    adc output+3
    sta output+3

    ;Decrease bit counter, continue if >0
    dex
    bne loop

    ;Go back to binary mode
    cld

    ;Set pointer to output, and we're done
    ldx #<output
    ldy #>output
 
    rts
.segment "VARS"
    input: .res 3
    output: .res 4
.CODE
.endproc

;******************************************************************************
;Function name.......: util_bcd_to_str
;Purpose.............: Converts a BCD value to a null terminated string
;Input...............: Pointer to 32 bit BCD value, X=AddressL, Y=AddressH
;Returns.............: Pointer to string, X=AddressL, Y=AddressH
;Errors..............: None
.proc util_bcd_to_str
    ;Copy input to local memory
    stx util_ptr
    sty util_ptr+1
    ldy #0

:   lda (util_ptr),y
    sta input,y
    iny
    cpy #5
    bne :-

    ldx #4      ;Byte index, start with rightmost byte (MSB)
    ldy #0      ;Char index, start from left

build_str:
    ;Get value from high nibble
    lda input-1,x       ;"input-1" is needed because X starts from 4 (i.e. last index+1); this makes the exit test simpler - maybe I'm optimizing too much?
    lsr
    lsr
    lsr
    lsr
    clc
    adc #48
    sta output,y
    iny

    ;Get value from low nibble
    lda input-1,x
    and #15
    clc
    adc #48
    sta output,y
    iny

    ;Decrease input byte index, if = 0 we're done
    dex
    bne build_str

strip_leading_zeros:
    ;Look for first non "0" value, and set the start of the output string to that position
    ldx #0
:   lda output,x
    cmp #48
    bne exit
    inx
    cpx #7          ;We must break this search at position 7. Otherwise 0 would be an empty string.
    bcc :-

exit:
    txa             ;Start of string from search for non 0 value above; added to the output pointer
    clc
    adc #<output
    tax
    lda #0
    adc #>output
    tay
 
    rts
.segment "VARS"
    input: .res 4
    output: .res 9
.CODE
.endproc
;BSD 2-Clause License
;
;Copyright (c) 2021, stefan-b-jakobsson
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
    token_ptr: .res 2
.CODE

token_last: .res 1

;******************************************************************************
;Function name: token_get
;Purpose......: Searches for a BASIC token ID in the file buffer
;Input........: X = Index where to start searching
;Output.......: A = token ID, X = extended token ID, Y = token length
;Errors.......: Nothing
.proc token_get
    ;Set initial values
    stx index
    
    stz len
    stz mismatch

    lda #$80
    sta token
    stz token+1
    
    lda #<tokens_std
    sta token_ptr
    lda #>tokens_std
    sta token_ptr+1

    ldy #0

loop:
    inc len
    lda (token_ptr),y
    beq exit                ;Token table is null terminated, stop here
    bmi lastchar            ;Bit 7 marks last char of a token in the token table

    cmp file_buf,x
    beq :+
    inc mismatch            ;Mark that there was a compare error

:   iny
    inx
    bra loop

lastchar:
    ;Compare token last char
    and #%01111111          ;Clear bit 7
    cmp file_buf,x
    bne next

    ;Check flag for previous mismatch
    lda mismatch
    bne next

    ;We have a match
    lda token
    ldx token+1
    ldy len
    rts

next:
    ;Move pointer to start of next token
    clc
    lda token_ptr
    adc len
    sta token_ptr
    lda token_ptr+1
    adc #0
    sta token_ptr+1

    ;Reset values
    stz mismatch
    stz len
    ldy #0
    ldx index

    ;Advcance token ID; if token+1 is not 0 we are looking for an extended token
    lda token+1
    beq :+

    inc token+1         ;Advance extended token ID
    bra loop

:   inc token           ;Advance standard token ID
    bra loop

exit:
    ;Have we searched for extended tokens?
    lda token+1
    bne :+          ;We have, exit function

    ;Set values to serach for extended tokens
    lda #$ce
    sta token
    lda #$80
    sta token+1

    lda #<tokens_ext
    sta token_ptr
    lda #>tokens_ext
    sta token_ptr+1

    stz mismatch
    stz len
    ldy #0
    ldx index
    bra loop

:   lda #0
    ldx #0
    ldy #0
    rts

index: .res 1
len: .res 1
token: .res 2
mismatch: .res 1
.endproc

tokens_std:
    ;CBM Basic V2 tokens: $80-8f
    .byt "en",$c4
    .byt "fo",$d2
    .byt "nex",$d4
    .byt "dat",$c1
    .byt "input",$a3
    .byt "inpu",$d4
    .byt "di",$cd
    .byt "rea",$c4
    .byt "le",$d4
    .byt "got",$cf
    .byt "ru",$ce
    .byt "i",$c6
    .byt "restor",$c5
    .byt "gosu",$c2
    .byt "retur",$ce
    .byt "re",$cd

    ;CBM Basic V2 tokens: $90-9f
    .byt "sto",$d0
    .byt "o",$ce
    .byt "wai",$d4
    .byt "loa",$c4
    .byt "sav",$c5
    .byt "verif",$d9
    .byt "de",$c6
    .byt "pok",$c5
    .byt "print",$a3
    .byt "prin",$d4
    .byt "con",$d4
    .byt "lis",$d4
    .byt "cl",$d2
    .byt "cm",$c4
    .byt "sy",$d3
    .byt "ope",$ce

    ;CBM Basic V2 tokens: $a0-af
    .byt "clos",$c5
    .byt "ge",$d4
    .byt "ne",$d7
    .byt "tab",$a8
    .byt "t",$cf
    .byt "f",$ce
    .byt "spc",$a8
    .byt "the",$ce
    .byt "no",$d4
    .byt "ste",$d0
    .byt $ab
    .byt $ad
    .byt $aa
    .byt $af
    .byt $de
    .byt "an",$c4

    ;CBM Basic V2 tokens: $b0-bf
    .byt "o",$d2
    .byt 190
    .byt $bd
    .byt 188
    .byt "sg",$ce
    .byt "in",$d4
    .byt "ab",$d3
    .byt "us",$d2
    .byt "fr",$c5
    .byt "po",$d3
    .byt "sq",$d2
    .byt "rn",$c4
    .byt "lo",$c7
    .byt "ex",$d0
    .byt "co",$d3
    .byt "si",$ce

    ;CBM Basic V2 tokens: $c0-cb
    .byt "ta",$ce
    .byt "at",$ce
    .byt "pee",$cb
    .byt "le",$ce
    .byt "str",$a4
    .byt "va",$cc
    .byt "as",$c3
    .byt "chr",$a4
    .byt "left",$a4
    .byt "right",$a4
    .byt "mid",$a4
    .byt "g",$cf

    .byt 0                  ;End of list

tokens_ext:
    ;Extended tokens: $ce + 80-8f
    .byt "mo", 'n' + $80
    .byt "do", 's' + $80
    .byt "ol", 'd' + $80
    .byt "geo", 's' + $80
    .byt "vpok", 'e' + $80
    .byt "vloa", 'd' + $80
    .byt "scree", 'n' + $80
    .byt "pse", 't' + $80
    .byt "lin", 'e' + $80
    .byt "fram", 'e' + $80
    .byt "rec", 't' + $80
    .byt "cha", 'r' + $80
    .byt "mous", 'e' + $80
    .byt "colo", 'r' + $80
    .byt "tes", 't' + $80
    .byt "rese", 't' + $80

    ;Extended tokens: $ce + 90-97
    .byt "cl", 's' + $80
    .byt "vpee", 'k' + $80
    .byt "m", 'x' + $80
    .byt "m", 'y' + $80
    .byt "m", 'b' + $80
    .byt "jo", 'y' + $80
    .byt "hex", $a4
    .byt "bin", $a4

    .byt 0              ;End of list
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
    token_ptr: .res 2
.CODE

.segment "VARS"
    token_last: .res 1
    token_count_std: .res 1
    token_count_ext: .res 1
    token_maxlen: .res 1
    token_cs_tbl: .res 7
.CODE

;******************************************************************************
;Function name: token_init
;Purpose......: Initializes token functions
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc token_init
    lda #1
    sta RAM_SEL_R38
    sta RAM_SEL_R39
    
    lda #<tokens_std
    sta token_ptr
    lda #>tokens_std
    sta token_ptr+1

    stz token_maxlen
    stz extended
    
    ldy #0
    ldx #0
    stz cs

loop:
    lda (token_ptr),y
    beq endoflist
    bmi endoftoken
    clc
    adc cs
    sta cs
    iny
    bra loop

endoftoken:
    and #%01111111
    clc
    adc cs
    sta $a000,x
    
    iny
    tya
    sta len
    sta $a100,x

    lda token_ptr
    sta $a200,x
    lda token_ptr+1
    sta $a300,x

    clc
    lda token_ptr
    adc len
    sta token_ptr
    lda token_ptr+1
    adc #0
    sta token_ptr+1

    lda len
    cmp token_maxlen
    bcc :+
    sta token_maxlen

:   ldy #0
    inx
    stz cs
    bra loop

endoflist:
    lda extended
    bne :+
    
    stx token_count_std
    
    inc extended
    lda #<tokens_ext
    sta token_ptr
    lda #>tokens_ext
    sta token_ptr+1
    
    ldy #0
    stz cs
    bra loop

:   stx token_count_ext
    rts

.segment "VARS"
    cs: .res 1
    len: .res 1
    extended: .res 1
.CODE
.endproc

;******************************************************************************
;Function name: token_get
;Purpose......: Searches for a BASIC token ID in the file buffer
;Input........: X = Index where to start searching
;Output.......: A = token ID, X = extended token ID, Y = token length
;Errors.......: Nothing
.proc token_get
    ;Init
    stx index
    stz cs

    lda #1
    sta RAM_SEL_R38
    sta RAM_SEL_R39

    ;Calculate checksum table
    ldy #0
:   lda file_buf,x
    beq :+
    cmp #32
    beq :+
    clc
    adc cs
    sta cs
    sta token_cs_tbl,y
    inx
    iny
    cpy token_maxlen
    bne :-

:   stx possible_len
    
    ldx #0
loop:
    ;Test len
    lda $a100,x
    sta len
    cmp possible_len
    beq :+
    bcs nexttoken

    ;Test checksum
:   tay
    lda $a000,x
    cmp token_cs_tbl-1,y
    bne nexttoken

    ;Compare name
    lda $a200,x
    sta token_ptr
    lda $a300,x
    sta token_ptr+1

    stx tokenid

    ldy #0
    ldx index
:   lda (token_ptr),y
    and #%01111111
    cmp file_buf,x
    bne :+
    inx
    iny
    cpy len
    beq ismatch
    bra :-

:   ldx tokenid

nexttoken:
    inx
    cpx token_count_ext
    bcc loop

    ldx #0
    ldy #0
    lda #0

    rts

ismatch:
    lda tokenid

    cmp token_count_std
    bcs :+
    
    clc
    adc #$80
    ldx #0
    ldy len
    rts

:   sec
    sbc token_count_std
    clc
    adc #$80
    tax
    lda #$ce
    ldy len
    rts

.segment "VARS"
    index: .res 1
    len: .res 1
    cs: .res 1
    possible_len: .res 1
    tokenid: .res 1
.CODE
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
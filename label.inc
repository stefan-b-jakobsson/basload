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

;******************************************************************************
;Notes on the symbol table used to store labels
;
;The symbol table is stored in banked RAM starting at bank 2.
;A checksum is calculated for each label (sum of ASCII values of the label
;name). The lower bits of the checksum are used to decide what "bucket" to
;store the data in. A bucket contains 256 symbols. Each bank contains two
;buckets, one starting at $a000 and one at $b000.
;
;Symbol table entry format is as follows:
;
;Offset  Byte count  Description
;------------------------------------------------------------------------------
;0       12          Symbol name
;12       1          Symbol length
;13       1          Checksum
;14       2          Line number
;
;The number of buckets used are set by the constant label_bucket_count below

LABEL_BUCKET_COUNT = 8      ;Must be even number >= 2

.ZEROPAGE
    label_ptr: .res 2
.CODE

.segment "VARS"
    label_count: .res LABEL_BUCKET_COUNT
.CODE

;******************************************************************************
;Function name: label_init
;Purpose......: Initializes label functions
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc label_init
    ldx #0
    lda #0

:   sta label_count,x
    inx
    cpx #LABEL_BUCKET_COUNT
    bne :-

    rts
.endproc

;******************************************************************************
;Function name: label_add
;Purpose......: Adds a label to the symbol table
;Input........: X = Index where label begins
;Output.......: A = result code: 0 = OK, 1 = not found, 2 = duplicate, 
;               3 = mem full
;               Y = label length
;Errors.......: Nothing
.proc label_add
    stx index
    stz len

    ;Validate format
    jsr label_val_def
    bne :+

    ;No label found
    lda #1
    rts

    ;Store checksum and len
:   sta cs
    sty len

    ;Check duplicate defintions
    ldx index
    jsr label_get
    cmp #0
    beq :+

    lda #2
    rts

:   ;Get bucket number
    lda cs
    and #(LABEL_BUCKET_COUNT-1)
    sta bucket

    ;Check if bucket full
    ldx bucket
    lda label_count,x
    cmp #$ff
    bne :+
    lda #3
    rts

    ;Store label name
:   ldy #0
    ldx index
:   lda file_buf,x
    sta (label_ptr),y
    iny
    inx
    cpy #12
    bcs :+
    cpy len
    bcc :-

    ;Store label len
:   ldy #12
    lda len
    sta (label_ptr),y

    ;Store checksum
    ldy #13
    lda cs
    sta (label_ptr),y

    ;Store line number
    ldy #14
    lda line_dstlin
    sta (label_ptr),y
    ldy #15
    lda line_dstlin+1
    sta (label_ptr),y

    ;Increase bucket counter
    ldx bucket
    inc label_count,x

    ;Set return values5
    lda #0
    ldy len
    rts

.segment "VARS"
    index: .res 1
    len: .res 1
    cs: .res 1
    bucket: .res 1
.CODE
.endproc

;******************************************************************************
;Function name: label_get
;Purpose......: Gets a label from the symbol table
;Input........: X = Index where label begins
;Output.......: Line number, X = LSB and Y = MSB
;               A = label length or 0 if not found
;Errors.......: Nothing
.proc label_get
    stx index

    jsr label_val_ref
    sta cs
    sty len

    cpy #0
    bne :+

    ;Invalid format, return A=0
    lda #0
    rts

:   ;Select RAM bank for the bucket
    lda cs
    and #7
    sta bucket
    
    lsr
    ina
    ina
    sta RAM_SEL_R38
    sta RAM_SEL_R39

    ;Set pointer to start of bucket
    stz label_ptr

    lda bucket
    and #1
    bne :+

    lda #$a0
    sta label_ptr+1
    bra :++

:   lda #$b0
    sta label_ptr+1

    ;Check if bucket is empty
:   ldx bucket
    lda label_count,x
    cmp #0
    bne :+
    lda #0
    rts

:   stz bucket_index

loop:
    ;Validate checksum
    ldy #13
    lda (label_ptr),y
    cmp cs
    bne next

    ;Validate length
    dey
    lda (label_ptr),y
    cmp len
    bne next

    ;Check name
    ldy #0
    ldx index
:   
    lda (label_ptr),y
    cmp file_buf,x
    bne next
    inx
    iny
    cpy #12
    bcs found
    cpy len
    bne :-

found:
    ldy #14
    lda (label_ptr),y
    tax
    iny
    lda (label_ptr),y
    tay
    
    lda len
    rts

next:
    clc
    lda label_ptr
    adc #16
    sta label_ptr
    lda label_ptr+1
    adc #0
    sta label_ptr+1

    inc bucket_index
    lda bucket_index
    ldx bucket
    cmp label_count,x
    bcc :+

    lda #0
    rts

:   ldx index
    ldy #0

    bra loop

.segment "VARS"
    index: .res 1
    len: .res 1
    cs: .res 1
    bucket: .res 1
    bucket_index: .res 1
.CODE
.endproc

;******************************************************************************
;Function name: label_val_def
;Purpose......: Validates the format of a possible label definition
;Input........: X = Index where label begins
;Output.......: A = Checksum
;               Y = Label length or 0 if invalid format 
;Errors.......: Nothing
.proc label_val_def
    stx index
    stz len
    stz cs

    ;Check first char is A-Z
    lda file_buf,x
    cmp #$41
    bcc invalid
    cmp #$5b
    bcs invalid
    
    sta cs
    inx
    inc len

    ;Check subsequent chars are A-Z or 0-9 ending with a colon
loop:
    lda file_buf,x
    
    cmp #$3a        ;Successfully reaching colon, return valid label def
    beq valid

    cmp #$30
    bcc invalid
    cmp #$3a
    bcc next
    cmp #$41
    bcc invalid
    cmp #$5b
    bcs invalid

next:
    clc
    adc cs
    sta cs

    inx
    inc len
    bra loop

valid:
    lda cs
    ldy len
    rts

invalid:
    lda cs
    lda len
    lda #0
    ldy #0
    rts

.segment "VARS"
    index: .res 1
    len: .res 1
    cs: .res 1
.CODE
.endproc

;******************************************************************************
;Function name: label_val_ref
;Purpose......: Validates the format of a possible label reference
;Input........: X = Index where label begins
;Output.......: Y = Label length, or 0 if invalid format
;               A = Checksum
;Errors.......: Nothing
.proc label_val_ref
    stx index
    stz len
    stz cs

    ;Check first char is A-Z
    lda file_buf,x
    cmp #$41
    bcc exit
    cmp #$5b
    bcs exit
    
    sta cs
    inx
    inc len

    ;Check subsequent chars are A-Z or 0-9
loop:
    lda file_buf,x
    cmp #$30
    bcc exit
    cmp #$3a
    bcc next
    cmp #$41
    bcc exit
    cmp #$5b
    bcs exit

next:
    clc
    adc cs
    sta cs

    inx
    inc len
    bra loop

exit:
    lda cs
    ldy len
    rts

.segment "VARS"
    index: .res 1
    len: .res 1
    cs: .res 1
.CODE
.endproc
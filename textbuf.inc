;BSD 2-Clause License
;
;Copyright (c) 2021-2022, Stefan Jakobsson
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
;Function name: textbuf_init
;Purpose......: Initializes pointers to read from start of X16 Edit text
;               buffer
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc textbuf_init
    ;AdressL + AddressH
    stz textbuf_ptr
    lda #$a0
    sta textbuf_ptr+1
    
    ;Bank
    lda #(LABEL_BUCKET_COUNT/2+3)
    sta textbuf_bnk

    ;Index
    stz textbuf_idx
    
    rts
.endproc

;******************************************************************************
;Function name: textbuf_readln
;Purpose......: Reads one line from the X16 Edit text buffer in banked RAM and 
;               stores it in the "file_buf" buffer
;Input........: Nothing
;Output.......: A = 0 if OK but not EOF
;               A = 1 if OK and EOF
;Errors.......: A = 2 if error reading line
.proc textbuf_readln
    ;Select RAM bank
    lda textbuf_bnk
    sta RAM_SEL
    
    ;Get page len
    stz textbuf_ptr
    ldy #4
    lda (textbuf_ptr),y
    sta len

    ;Skip metadata
    lda #5
    sta textbuf_ptr

    ;Offset
    ldy textbuf_idx
    ldx #0

loop:
    cpy len
    bcs nextpage

    lda (textbuf_ptr),y
    cmp #10
    beq eol

    sta file_buf,x
    iny
    inx
    bra loop

nextpage:
    stz textbuf_ptr
    ldy #2
    lda (textbuf_ptr),y
    pha
    iny
    lda (textbuf_ptr),y
    beq eof
    sta textbuf_ptr+1
    pla
    sta textbuf_bnk
    sta RAM_SEL
    
    lda #5
    sta textbuf_ptr
    ldy #0
    bra loop

eol:
    iny
    sty textbuf_idx
    lda #0
    sta file_buf,x
    rts

eof:
    pla
    lda #0
    sta file_buf,x
    lda #1
    rts

.segment "VARS"
    len: .res 1
.CODE

.endproc

.ZEROPAGE
    textbuf_ptr: .res 2     ;AddressL, AddressH
    textbuf_bnk: .res 1
    textbuf_idx: .res 1
.CODE
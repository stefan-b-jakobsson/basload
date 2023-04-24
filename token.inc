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

.export token_ptr
.ZEROPAGE
    token_ptr: .res 2
.CODE

;******************************************************************************
;Function name: token_init
;Purpose......: Initializes token functions. This function mainly reads 
;               through all tokens storing a checksum, the length of the
;               token, a pointer to the token name start in
;               the list of tokens. This information is later used to
;               enhance token search performance.
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc token_init
    ;Store current ROM bank on stack, and select BASIC ROM bank (=4)
    lda ROM_SEL
    pha
    lda #4
    sta ROM_SEL

    lda #1
    sta RAM_SEL

    ;Get start of token lists in BASIC ROM
    ldx #<token_stdtbl_head
    ldy #>token_stdtbl_head
    jsr token_find_table
    stx token_stdtbl_start
    sty token_stdtbl_start+1

    ldx #<token_exttbl_head
    ldy #>token_exttbl_head
    jsr token_find_table
    stx token_exttbl_start
    sty token_exttbl_start+1

    ;Prepare reading through all standard tokens
    lda token_stdtbl_start
    sta token_ptr
    lda token_stdtbl_start+1
    sta token_ptr+1

    ;Prepare to gather token metadata
    stz token_maxlen        ;The length of the longest token
    stz extended_flag       ;A flag indicating whether extended tokens have been analyzed yey

    ldy #0
    ldx #0
    stz cs
loop:
    lda (token_ptr),y       ;Get current token char
    beq endoflist           ;The list of tokens is null terminated, check if we have reached the end
    bmi endoftoken          ;Bit 7=1 marks last char of a token
    
    clc                     ;It was not the last char
    adc cs
    sta cs                  ;Update checksum value
    iny
    bra loop                ;Continue

endoftoken:
    and #%01111111          ;Clear bit 7 which is used as end of token marker
    clc
    adc cs
    sta token_cs_tbl,x      ;Store final checksum for this token
    
    iny
    tya
    sta len
    sta token_len_tbl,x     ;Store token length

    lda token_ptr           ;Store pointer to token start in BASIC ROM (LSB and MSB)
    sta token_addressL_tbl,x
    lda token_ptr+1
    sta token_addressH_tbl,x

    ldy #0                  ;Check if first extended function
:   lda (token_ptr),y
    cmp token_ext_func_head,y
    bne :+
    iny
    cpy #5
    bne :-
    inc extended_flag
    stx token_count_ext_stm ;Store index of last extended statement

:   clc                     ;Move pointer to start of next token
    lda token_ptr
    adc len
    sta token_ptr
    lda token_ptr+1
    adc #0
    sta token_ptr+1

    lda len                 ;Update maxlen value, if this token is the longest so far
    cmp token_maxlen        ;We need this info to speed up token search later on
    bcc :+
    sta token_maxlen

:   ldy #0                  ;Prepare for next token
    inx                     ;Advance token id
    stz cs                  ;Reset checksum counter
    bra loop

endoflist:
    lda extended_flag       ;Check if we have read the extended tokens
    cmp #2                  ;2=extended functions have been read too
    bcs exit
    cmp #0
    bne next_ext_list
    
    stx token_count_std     ;Store number of standard tokens

    lda token_exttbl_start  ;Set pointer to start of token table in BASIC ROM
    sta token_ptr
    lda token_exttbl_start+1
    sta token_ptr+1
    
    ldy #0                  ;Prepare for next token
    stz cs                  ;Reset checksum counter

    inc extended_flag       ;Set flag that we now start to read extended tokens
    jmp loop

next_ext_list:
    ldy #0                  ;Prepare for next token
    stz cs                  ;Reset checksum counter
    inc token_ptr
    beq :+
    jmp loop
:   inc token_ptr+1
    jmp loop

exit:   
    stx token_count_ext_func     ;Store index of last extended function
    pla
    sta ROM_SEL
    rts

.segment "VARS"
    cs: .res 1
    len: .res 1
    extended_flag: .res 1   ;Flag to indicate if extended tokens have been analyzed
.CODE
.endproc

;******************************************************************************
;Function name: token_get
;Purpose......: Searches for a BASIC token in the file buffer
;Input........: X = Buffer index where to start searching
;Output.......: A = token ID, X = extended token ID, Y = token length
;               A, X, Y = 0 if no token found
;Errors.......: None
.proc token_get
    ;Init
    stx index
    stz cs

    ;Store current ROM bank on the stack, and set BASIC ROM bank (=4)
    lda #4
    sta ROM_SEL

    ;Select RAM bank 1 where token metadata is stored
    lda #1
    sta RAM_SEL

    ;Calculate and store checksums for all possible token lenghts
    ;Calculation is stopped (1) at a blank space (2) at end of line or (3) at the token_maxlen retrieved by the token_init function
    ldy #0
:   lda file_buf,x
    beq :+                  ;EOF
    cmp #32                 ;Blank space
    beq :+
    clc                     ;Update checksum
    adc cs
    sta cs
    sta cs_tbl,y
    inx
    iny
    cpy token_maxlen        ;At token maxlen?
    bne :-

:   sty possible_len        ;Store the longest possible token to look for in this case
    
    ldx #0                  ;X is token index
loop:
    ;Verify token len
    lda token_len_tbl,x
    sta len
    cmp possible_len
    beq :+
    bcs nexttoken           ;Token too long, check next token

    ;Verify token checksum
:   tay
    lda token_cs_tbl,x
    cmp cs_tbl-1,y
    bne nexttoken           ;Token checksum is different, check next token

    ;Compare name
    lda token_addressL_tbl,x
    sta token_ptr
    lda token_addressH_tbl,x
    sta token_ptr+1

    stx tokenid

    ldy #0
    ldx index
:   lda (token_ptr),y
    and #%01111111          ;Ensure bit 7 is cleared when comparing names
    cmp file_buf,x
    bne :+
    inx
    iny
    cpy len
    beq ismatch             ;We reached token len without match errors => we have a match
    bra :-

:   ldx tokenid

nexttoken:
    inx                         ;Increase token id
    cpx token_count_ext_func     ;If we reach this token id, we're done searching...
    bcc loop

nomatch:
    ldx #0                  ;... there was not match
    ldy #0
    lda #0

    rts

ismatch:
    lda tokenid

    ;Check if we got a standard or extended token
    cmp token_count_std
    bcs :+
    
    ;Standard token (same as Commodore)
    clc
    adc #$80
    ldx #0
    ldy len
    rts

    ;Extended token (X16 additions), implemented by two bytes, a prefix $ce, and then a token id
:   cmp token_count_ext_stm
    bcs :+

    sec                         ;Extended statement
    sbc token_count_std
    clc
    adc #$80        
    tax             
    lda #$ce 
    ldy len
    rts

:   sec                         ;Extended function
    sbc token_count_ext_stm
    clc
    adc #$d0
    tax
    lda #$ce
    ldy len
    rts

.segment "VARS"
    index: .res 1
    len: .res 1
    cs: .res 1
    cs_tbl: .res 7
    possible_len: .res 1
    tokenid: .res 1
    token_stdtbl_head_start: .res 2
    token_exttbl_head_start: .res 2
.CODE
.endproc

;******************************************************************************
;Function name: token_find_table
;Purpose......: Searches BASIC ROM for the start of standard and extended
;               token tables
;Input........: Pointer to string of 6 chars to search for (the start of
;               the list of tokens in BASIC ROM)
;Output.......: Pointer to start of table: X=AddressL, Y=AddressH
;Errors.......: X and Y = 0 if not found
.proc token_find_table
    ;Store input pointer address
    stx TEMP2
    sty TEMP2+1
    
    ;Setup pointer to start of ROM (=$c000)
    stz TEMP1
    lda #$c0
    sta TEMP1+1

    ;Search
    ldy #0
loop:
    lda (TEMP2),y
    cmp (TEMP1)
    bne no_match
    iny
    cpy #6
    bcs found

next:   
    inc TEMP1
    bne loop
    inc TEMP1+1
    bne loop

not_found:
    ldx #0
    ldy #0
    rts

no_match:
    ldy #0
    bra next

found:
    sec
    lda TEMP1
    sbc #5
    tax
    lda TEMP1+1
    sbc #0
    tay

    rts
.endproc

.segment "VARS"
    token_count_std: .res 1         ;Holds index of last standard token
    token_count_ext_stm: .res 1     ;Holds index of last extended statement
    token_count_ext_func: .res 1    ;Holds index of last extended function
    token_maxlen: .res 1            ;Holds the length of the longest token
    token_stdtbl_start: .res 2      ;Pointer to start of first standard token in BASIC ROM
    token_exttbl_start: .res 2      ;Pointer to start of first extended token in BASIC ROM
.CODE

token_cs_tbl        = $a000         ;Table of token checksums (255 bytes)
token_len_tbl       = $a100         ;Table of token lenghts (255 bytes)
token_addressL_tbl  = $a200         ;Table of pointers to start of individual tokens in BASIC ROM (AddressL)
token_addressH_tbl  = $a300         ;Table of pointers to start of individual tokens in BASIC ROM (AddressH)

token_stdtbl_head:
    .byt "en",$c4,"fo",$d2

token_exttbl_head:
    .byt "mo", 'n' + $80, "do", 's' + $80

token_ext_func_head:
    .byt "vpee", 'k' + $80
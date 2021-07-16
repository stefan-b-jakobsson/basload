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
    line_meta: .res 2
    line_code: .res 2
.CODE

.segment "VARS"
    line_srclin: .res 3
    line_dstlin: .res 2
.CODE

;******************************************************************************
;Function name: line_reset
;Purpose......: Resets all pointers to start values, to be called once before
;               pass 1 and once before pass 2
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc line_reset
    lda #$01
    sta line_meta
    lda #$05
    sta line_code
    lda #$08
    sta line_meta+1
    sta line_code+1

    lda #0
    ldy #0
    sta (line_meta),y
    iny
    sta (line_meta),y

    stz line_srclin
    stz line_srclin+1
    stz line_srclin+2

    stz line_dstlin
    stz line_dstlin+1

    rts
.endproc

;******************************************************************************
;Function name: line_pass1
;Purpose......: Load pass 1 function, looks for label defintions
;Input........: Nothing
;Output.......: Nothing
;Errors.......: A = 1 if error occured, else A = 0
.proc line_pass1
    ;Init
    stz index_src
    stz label_found
    stz count_chars
    stz count_chars_after_label

    ;Increase line numbers
    inc line_srclin
    bne :+
    inc line_srclin+1
    bne :+
    inc line_srclin+2

:   inc line_dstlin
    bne loop
    inc line_dstlin+1
    bne loop

    ldx #<msg_line_num_overflow         ;Line number overflow, i.e. greater than 65,535
    ldy #>msg_line_num_overflow
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

loop:
    ldx index_src
    lda file_buf,x

    bne checks

    ;Exit
eol:
    lda count_chars_after_label
    bne :++

    lda line_dstlin
    bne :+
    dec line_dstlin+1
:   dec line_dstlin

:   lda #0
    rts

checks:
    ;Check if blank space
    cmp #32
    bne :+ 
    jmp next
:   inc count_chars
    inc count_chars_after_label

    ;Check if label def
    ldy label_found
    beq :+
    jmp next

:   ldy count_chars
    cpy #1
    beq :+
    jmp next

    ;Check if token
:   ldx index_src               ;Get possible label def length
    jsr label_val_def
    sty len
    cpy #0
    beq next

    ldx index_src               ;Check if it's a reserved token
    jsr token_get
    cpy len
    beq next

    ldx index_src               ;Store label
    jsr label_add

    cmp #1                      ;Label not found in file_buf. Should not happen due to previous tests
    bne :+
    ldx #<msg_parse_error
    ldy #>msg_parse_error
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

:   cmp #2                      ;Duplicate label error
    bne :+
    ldx #<msg_duplicate_labels
    ldy #>msg_duplicate_labels
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

:   cmp #3                      ;Symbol table full
    bne :+
    ldx #<msg_mem_full
    ldy #>msg_mem_full
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

:   inc label_found             ;Label stored successfully
    stz count_chars_after_label
    clc
    lda index_src
    adc len
    sta index_src
    
    lda len
    cmp #13
    bcc next

    ldx #<msg_label_too_long
    ldy #>msg_label_too_long
    lda #%00010000
    jsr ui_msg

next:
    inc index_src
    jmp loop

.segment "VARS"
    index_src: .res 1
    label_found: .res 1
    count_chars: .res 1
    count_chars_after_label: .res 1
    len: .res 1
.CODE
.endproc

;******************************************************************************
;Function name: line_pass2
;Purpose......: Load pass 2 function, enters BASIC code into memory
;Input........: Nothing
;Output.......: Nothing
;Errors.......: A = 1 if error occured, else A = 0
.proc line_pass2
    ;Init
    stz last_token
    stz index_src
    stz index_dst
    stz count_quotes
    stz count_chars
    stz count_chars_after_label

    ;Increase line numbers
    inc line_srclin
    bne :+
    inc line_srclin+1
    bne :+
    inc line_srclin+2

:   inc line_dstlin
    bne loop
    inc line_dstlin+1
    bne loop

    ldx #<msg_line_num_overflow         ;Line number overflow, i.e. greater than 65,535
    ldy #>msg_line_num_overflow
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

loop:
    ldx index_src
    lda file_buf,x
    bne chk_rem

    ;End of line
    lda count_chars_after_label
    bne :++
    lda line_dstlin
    bne :+
    dec line_dstlin+1
:   dec line_dstlin
    lda #0
    rts
    
:   ldy index_dst
    lda #0                      ;Set end of line marker
    sta (line_code),y
    inc index_dst

    clc                         ;Address of start of next line
    lda line_code
    adc index_dst
    sta nxtlinptr
    lda line_code+1
    adc #0
    sta nxtlinptr+1

    ldy #0                      ;Set current line metadata: pointer to start of next line
    lda nxtlinptr
    sta (line_meta),y
    iny
    lda nxtlinptr+1
    sta (line_meta),y

    iny                         ;Set current line metadata: line number
    lda line_dstlin
    sta (line_meta),y
    sta KERNAL_LINNUM
    iny
    lda line_dstlin+1
    sta KERNAL_LINNUM+1
    sta (line_meta),y

    lda nxtlinptr
    sta line_meta
    lda nxtlinptr+1
    sta line_meta+1
    
    clc
    lda nxtlinptr
    adc #4
    sta line_code
    sta KERNAL_VARTAB
    lda nxtlinptr+1
    adc #0
    sta line_code+1
    sta KERNAL_VARTAB+1

    lda #0                      
    ldy #0
    sta (line_meta),y
    iny
    sta (line_meta),y

    lda #0                      ;No error
    rts

chk_rem:
    ldy last_token
    cpy #$8f
    bne chk_blnk
    jmp copy

chk_blnk:
    cmp #32
    bne :++
    
    lda count_quotes
    and #1
    bne :+
    jmp next
:   jmp copy

:   inc count_chars
    inc count_chars_after_label

chk_quote:
    cmp #34
    bne chk_instr
    inc count_quotes
    jmp copy

chk_instr:
    lda count_quotes
    and #1
    beq chk_labeldef
    jmp copy                    ;Within strings, we just copy input to destination

chk_labeldef:
    lda count_chars
    cmp #1
    bne chk_labelref

    ldx index_src
    jsr label_get
    cmp #0
    beq chk_labelref

    clc
    adc index_src
    ina
    sta index_src
    stz count_chars_after_label
    jmp loop

chk_labelref:
    lda last_token
    cmp #$89                    ;GOTO
    beq :+
    cmp #$8d                    ;GOSUB
    bne chk_token

:   ldx index_src
    jsr label_get
    cmp #0
    bne :+

    ldx #<msg_label_expected
    ldy #>msg_label_expected
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

:   pha                         ;Store label length on stack

    lda #0
    jsr util_bin_to_bcd
    jsr util_bcd_to_str
    
    stx util_ptr
    sty util_ptr+1
    ldx #0

:   txa
    tay 
    lda (util_ptr),y
    beq :+
    ldy index_dst
    sta (line_code),y
    inc index_dst
    inx
    bra :-

:   clc
    pla
    adc index_src
    sta index_src

    stz last_token
    jmp loop
    
chk_token:
    ldx index_src
    jsr token_get
    cmp #0
    beq copy

    phy                         ;Store token length on stack

    ldy index_dst               ;Store standard token
    sta (line_code),y
    sta last_token
    inc index_dst

    cmp #$ce                    ;Store extended token, if applicable
    bne :+
    ldy index_dst
    txa
    sta (line_code),y
    inc index_dst

:   clc                         ;Move src index forward
    pla                         ;Pull token length from stack
    adc index_src
    sta index_src
    jmp loop

copy:
    ldx index_src
    ldy index_dst
    lda file_buf,X
    sta (line_code),y
    inc index_dst

next:
    inc index_src
    jmp loop

.segment "VARS"
    index_src: .res 1
    index_dst: .res 1
    nxtlinptr: .res 2
    count_quotes: .res 1
    count_chars: .res 1
    count_chars_after_label: .res 1
    last_token: .res 1
.CODE
.endproc
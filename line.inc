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

.ZEROPAGE
    line_meta: .res 2
    line_code: .res 2
.CODE

.segment "VARS"
    line_srclin: .res 3
    line_dstlin: .res 2
.CODE

REM_STMT = $8f
THEN_STMT = $a7
RESTORE_STMT = $8c
GOTO_STMT = $89
GOSUB_STMT = $8d
DATA_STMT = $83

;******************************************************************************
;Function name: line_init
;Purpose......: Initializes all pointers to start values, to be called once 
;               before pass 1 and once before pass 2
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc line_init
    ;Set pointers to start of BASIC memory
    lda #$01
    sta line_meta
    lda #$05
    sta line_code
    lda #$08
    sta line_meta+1
    sta line_code+1

    ;Mark end of program at start of BASIC memory
    lda #0
    ldy #0
    sta (line_meta),y
    iny
    sta (line_meta),y

    ;Init source line counter
    stz line_srclin
    stz line_srclin+1
    stz line_srclin+2

    ;Init destination line counter
    stz line_dstlin
    stz line_dstlin+1

    rts
.endproc

;******************************************************************************
;Function name: line_pass1
;Purpose......: Load pass 1; looks for label defintions and stores them in
;               the symbol table
;Input........: Nothing
;Output.......: A = 0 if OK
;Errors.......: A = 1 if error occured
.proc line_pass1
    ;Init
    stz index_src
    stz label_found
    stz count_chars
    stz count_chars_after_label
    stz lasttoken

    ;Increase line numbers
    inc line_srclin
    bne :+
    inc line_srclin+1
    bne :+
    inc line_srclin+2
    beq :++

:   inc line_dstlin
    bne loop
    inc line_dstlin+1
    bne loop

:   ldx #<msg_line_num_overflow         ;Line number overflow, i.e. greater than 65,535
    ldy #>msg_line_num_overflow
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

loop:
    lda count_chars_after_label         ;If we have two chars or more after label we need not look any further in pass 1
    cmp #2
    bcs eol

    ldx index_src                       ;Get next char
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
    ldy lasttoken               ;Check if within REM statement
    cpy #REM_STMT
    bne :+
    jmp next

:   cmp #32                     ;Check if white space
    bne :+
    jmp next
:   cmp #9
    bne :+
    jmp next
:   inc count_chars
    inc count_chars_after_label

    ;Check if label def
    ldy label_found
    beq :+
    jmp chk_token

:   ldy count_chars             ;Check if first non white space char
    cpy #1
    beq :+
    jmp chk_token

:   ldx index_src               ;Get possible label def length
    jsr label_val_def
    sty len
    cpy #0                      ;Not a label definition
    bne :+
    jmp chk_token

:   ldx index_src               ;Check if it's a reserved token
    jsr token_get
    cpy len
    bne store_label             ;A possible token is not the same length, treat it as a label def

    sta lasttoken               ;It was a token
    cmp #REM_STMT               ;Is it a REM statement?
    beq :+
    jmp next
:   dec count_chars_after_label
    bne :+
    jmp next
    
:   ldx index_src               ;Look for a possible colon before REM statement
    lda file_buf-1,x
    cmp #':'
    bne next
    dec count_chars_after_label
    bra next

store_label:
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
    bra next

chk_token:
    ldx index_src               ;Check if REM token
    jsr token_get
    beq next
    
    sta lasttoken

    cmp #REM_STMT
    bne next
    dec count_chars_after_label
    beq next

    ldx index_src
    lda file_buf-1,x
    cmp #':'
    bne next
    dec count_chars_after_label

next:
    inc index_src
    jmp loop

.segment "VARS"
    index_src: .res 1
    label_found: .res 1
    count_chars: .res 1
    count_chars_after_label: .res 1
    len: .res 1
    lasttoken: .res 1
.CODE
.endproc

;******************************************************************************
;Function name: line_pass2
;Purpose......: Load pass 2; tokenizes the source code and replaces label
;               references with the corresponding line numbers stored in
;               the symbol table during pass 1
;Input........: Nothing
;Output.......: A = 0 if OK
;Errors.......: A = 1 if error occured
.proc line_pass2
    ;Init
    stz last_token
    stz index_src
    stz index_dst
    stz count_quotes
    stz count_chars
    stz count_labels

    ;Increase source line counter
    inc line_srclin
    bne :+
    inc line_srclin+1
    bne :+
    inc line_srclin+2
    beq :++                         ;Line overflow

    ;Increase destination line counter
:   inc line_dstlin
    bne loop
    inc line_dstlin+1
    bne loop

    ;Line number overflow
:   ldx #<msg_line_num_overflow
    ldy #>msg_line_num_overflow
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

loop:
    ldx index_src
    lda file_buf,x
    bne chk_rem
    jmp eol                 ;Last char was 0 => end of line

chk_rem:                    ;Ignore REM statements
    ldy last_token
    cpy #REM_STMT
    bne chk_data
    jmp next

chk_data:                   
    cpy #DATA_STMT          ;Was last token DATA?
    bne chk_blnk
    tay
    lda count_quotes        ;Are we in quote mode?
    lsr
    bcs :+
    tya
    cmp #':'                ;No, a colon should break the DATA statement
    bne :+
    stz last_token          ;Reset last_token after colon
:   jmp copy

chk_blnk:                   ;Check if last char was a blank space or tab; if yes ignore if outside string, copy to destination if within string
    cmp #32
    beq :+
    cmp #9
    bne :+++
    
:   lda count_quotes
    and #1
    bne :+
    jmp next
:   jmp copy

:   inc count_chars

chk_quote:                  ;Check if last char was a double quote; if yes, increase counter
    cmp #34
    bne chk_instr
    inc count_quotes
    jmp copy

chk_instr:                  ;Check if we are within a string; if yes, just copy char to destination unaltered
    lda count_quotes
    and #1
    beq chk_labeldef
    jmp copy

chk_labeldef:               ;Check if we are at start of a label definition; if yes, skip over that
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
    jmp loop

chk_labelref:                   ;After GOTO, GOSUB, THEN, and RESTORE there may be a label reference, check that
    lda last_token
    cmp #THEN_STMT              ;THEN token id
    beq then_restore_label
    cmp #RESTORE_STMT           ;RESTORE token id
    beq then_restore_label
    cmp #GOTO_STMT              ;GOTO token id
    beq :+
    cmp #GOSUB_STMT             ;GOSUB token id
    beq :+
    jmp chk_token

:   lda count_labels            ;Check label count, if even look for label def, else look for comma delimiter between GOTO/GOSUB targets
    and #1
    beq :++

    ldx index_src               ;Here testing for comma delimiter
    lda file_buf,x
    cmp #','
    beq :+
    stz last_token              ;Was not a comma, clear last token so we will look no further

:   inc count_labels            ;Increase counter to make it even again
    jmp copy

:   ldx index_src               ;Get label
    jsr label_get
    cmp #0
    bne :+

    ldx #<msg_label_expected    ;Label not found error
    ldy #>msg_label_expected
    lda #%00010001
    jsr ui_msg
    lda #1
    rts

:   pha                         ;Store label length on stack

    lda #0                      ;Output target line number
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

:   clc                         ;Move source code pointer forward
    pla
    adc index_src
    sta index_src

    inc count_labels
    jmp loop

then_restore_label:             ;After THEN there can be a label ref or any other statement; after RESTORE there can also be a label ref, therefore different to GOTO/GOSUB where there must be a label ref
    ldx index_src
    jsr label_get
    cmp #0
    bne :+
    stz last_token
    bra chk_token

:   pha                         ;Store label length on stack

    lda #0                      ;Output target line number
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

:   clc                         ;Move source code pointer forward
    pla
    adc index_src
    sta index_src

    jmp loop
    
chk_token:                      ;Check if we are at start of a BASIC command that is to be translated to a token
    ldx index_src
    jsr token_get
    cmp #0
    beq copy

    phy                         ;Store token length on stack
    sta last_token              ;Remember last token
    
    cmp #REM_STMT               ;Ignore REM token
    bne :+

    ldy index_dst               ;Remove possible colon before REM statement
    beq :++
    dey
    lda (line_code),y
    cmp #':'
    bne :++
    sty index_dst
    bra :++

:   ldy index_dst               ;Store standard token
    sta (line_code),y
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

copy:                           ;This copies the last character to the destination unaltered
    ldx index_src
    ldy index_dst
    lda file_buf,X
    sta (line_code),y
    inc index_dst

next:                           ;Moves to next char on the current line
    inc index_src
    jmp loop

eol:                            ;End of line handling, 
    lda index_dst
    bne :++
    
    lda line_dstlin             ;No BASIC statement on the current line (blank or only label definition) - decrease destination line counter as we are not outputting anything
    bne :+
    dec line_dstlin+1
:   dec line_dstlin
    lda #0
    rts
    
:   ldy index_dst               ;Set end of line marker
    lda #0                     
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
    lda nxtlinptr+1
    adc #0
    sta line_code+1

    lda #0                      
    ldy #0
    sta (line_meta),y
    iny
    sta (line_meta),y

    lda #0                      ;No error
    rts

.segment "VARS"
    index_src: .res 1
    index_dst: .res 1
    nxtlinptr: .res 2
    count_quotes: .res 1
    count_chars: .res 1
    count_labels: .res 1
    last_token: .res 1
.CODE
.endproc

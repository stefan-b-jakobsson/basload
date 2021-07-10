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

.include "common.inc"

jsr main_greeting
jsr main_get_sourcefile
jsr main_load
rts

;******************************************************************************
;Function name: main_greeting
;Purpose......: Prints program greeting
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_greeting
    ldx #<ps
    ldy #>ps
    jsr ui_print
    rts

    ps: .byt 13,"*** basic loader 0.0.1 ***",13, "(c) 2021 stefan jakobsson",13,13,"source file name: ",0

.endproc

;******************************************************************************
;Function name: main_get_sourcefile
;Purpose......: Prompts the user for the source file name
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_get_sourcefile
    ldy #0

:   jsr KERNAL_CHRIN
    cmp #13
    beq eol
    sta file_name,y
    iny
    bra :-

eol:
    iny
    sty file_len

    ldx #<msg
    ldy #>msg
    jsr ui_print
    rts

msg: .byt   13, "loading...", 13, 0
.endproc

;******************************************************************************
;Function name: main_load
;Purpose......: Loads the source file
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_load
    jsr line_reset
    jsr label_init

    jsr file_open
    cmp #0
    bne err1

:   jsr file_readln
    cmp #1
    beq eof1
    cmp #2
    beq err1

    jsr line_pass1
    cmp #0
    beq :-

err1:
    jsr file_close
    jmp main_print_disk_status

eof1:
    jsr line_pass1
    
    ;Pass 2
    jsr line_reset
    jsr file_open

pass2_loop:
    jsr file_readln
    cmp #1
    beq eof2
    cmp #2
    beq err2

    jsr line_pass2
    bra pass2_loop

err2:
    jsr file_close
    jmp main_print_disk_status

eof2:
    jsr line_pass2
    jsr file_close
    rts

.endproc

;******************************************************************************
;Function name: main_print_disk_status
;Purpose......: Displays disk status
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_print_disk_status
    lda file_err
    beq :+

    tax
    dex
    lda file_ioerr_H,x
    tay
    lda file_ioerr_L,x
    tax
    lda #%00000001
    jsr ui_msg

    stz file_err
    rts

:   jsr file_status
    cmp #0
    beq :+

    ldx #<(file_buf)
    ldy #>(file_buf)
    lda #%00000001
    jsr ui_msg

:   rts

.endproc

.include "file.inc"
.include "ui.inc"
.include "line.inc"
.include "token.inc"
.include "label.inc"
.include "util.inc"
.include "msg.inc"
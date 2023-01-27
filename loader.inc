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
;Function name: loader_prompt
;Purpose......: Default entry point, starts basic loader
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc loader_prompt
    ;Select Kernal ROM bank
    lda ROM_SEL
    pha
    stz ROM_SEL

    ;Display program greeting
    ldx #<msg
    ldy #>msg
    jsr ui_print

    ;Prompt user for source file
    jsr loader_get_sourcefile
    lda file_len
    beq exit

    ;Set load flag = true
    lda #1
    sta loader_loadflag

    ;Start loading...
    jsr loader_run
    
    ;We're done, reset ROM bank to its original value
exit:
    pla     
    sta ROM_SEL
    rts

    msg: .byt 13, .sprintf("*** basload %u.%u.%u ***", appversion_major, appversion_minor, appversion_patch),13, 0
.endproc

;******************************************************************************
;Function name: loader_get_sourcefile
;Purpose......: Prompts the user for a source file name to be loaded
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc loader_get_sourcefile
    ;Prompt to load last file
    lda loader_loadflag
    beq prompt
    lda file_len
    beq prompt

    ldx #<msg_loadprev
    ldy #>msg_loadprev
    jsr ui_print
    
    ldy file_len
    lda #0
    sta file_name,y
    ldx #<file_name
    ldy #>file_name
    jsr ui_print

    ldx #<msg_loadprev2
    ldy #>msg_loadprev2
    jsr ui_print

    ldy #0
:   jsr KERNAL_CHRIN
    cmp #13
    beq :+
    sta file_buf,y
    iny
    bra :-

:   cpy #0
    beq reload
    lda file_buf
    and #%11011111
    cmp #'y'
    beq reload
    cmp #'n'
    beq prompt
    bra loader_get_sourcefile     ;Invalid response, try again

reload:
    ldx #<msg_loading
    ldy #>msg_loading
    jsr ui_print
    rts

    ;Prompt user for new file name
prompt:
    ldx #<msg_prompt
    ldy #>msg_prompt
    jsr ui_print

    ldy #0
:   jsr KERNAL_CHRIN
    cmp #13
    beq eol
    sta file_name,y
    iny
    bra :-

eol:
    sty file_len

    cpy #0
    beq noinput

    ldx #<msg_loading
    ldy #>msg_loading
    jsr ui_print
    rts

noinput:
    ldx #<msg_nofile
    ldy #>msg_nofile
    jsr ui_print
    rts

msg_loadprev: .byt 13, "last file was ",0
msg_loadprev2: .byt 13, "reload (cr=y/n)? ",0

msg_prompt: .byt 13, "enter file name: ", 0

msg_loading: .byt   13, "loading...", 0
msg_nofile: .byt 13, "no source file", 0
.endproc

;******************************************************************************
;Function name: loader_run
;Purpose......: Loads the source file
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc loader_run
    ;Prepare pass 1
    jsr token_init
    jsr line_init
    jsr label_init

    jsr file_init
    jsr file_open
    cmp #0
    bne err1

    ;Read and process each line
:   jsr loader_readln
    cmp #1
    beq eof1
    cmp #2
    beq err1

    jsr line_pass1
    cmp #0
    beq :-

err1:
    jsr file_close
    jmp file_print_disk_status

eof1:
    jsr line_pass1
    cmp #0
    bne err1
    
    ;Prepare pass 2
    jsr line_init

    ;Open source file
    jsr file_open

    ;Read and process each line again
pass2_loop:
    jsr loader_readln
    cmp #1
    beq eof2
    cmp #2
    beq err2

    jsr line_pass2
    bra pass2_loop

err2:
    jsr file_close
    jmp file_print_disk_status

eof2:
    jsr line_pass2
    cmp #0
    bne err2

    ;We're done
    jsr file_close
    rts

.endproc

;******************************************************************************
;Function name: loader_readlin
;Purpose......: Reads one line from the current input source and stores it
;               into file_buffer
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc loader_readln
    ;For now there is only one possible input source
    jmp file_readln
.endproc

.segment "VARS"
    loader_loadflag: .byt 0
.CODE
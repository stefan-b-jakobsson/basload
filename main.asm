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

.include "common.inc"

;******************************************************************************
;Jump table

    ;$9000
    jmp main_default
    
    ;$9003
    jmp main_editor

.proc main_default
    ;Initialize
    jsr main_init

    ;Select Kernal ROM bank
    lda (ROM_SEL)
    pha
    lda #0
    sta (ROM_SEL)

    ;Display program info and ask user for a source file
    jsr main_greeting
    jsr main_get_sourcefile
    lda file_len
    beq exit

    ;Set load flag = true
    lda #1
    sta main_loadflag

    ;Start loading...
    jsr main_load
    
    ;We're done, reset ROM bank to its original value
exit:
    pla     
    sta (ROM_SEL)
    rts
.endproc

;******************************************************************************
;Function name: main_init
;Purpose......: Initializes main functions
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_init
    ;Need to declare this beforehand, otherwise the compiler doesn't know it's in ZP
    .ZEROPAGE
        fv: .res 2
    .CODE

    ;Get Kernal version (bank 0/$ff80), we must use Kernal FETVEC function as we do not yet know how to set the ROM bank
    lda #<KERNAL_VERSION
    sta fv
    lda #>KERNAL_VERSION
    sta fv+1
    ldx #0
    ldy #0
    lda #fv
    jsr KERNAL_FETVEC

    cmp #$da
    bne :+
    
    lda #$61                     ;Version: R38
    sta RAM_SEL
    lda #$60
    sta ROM_SEL
    lda #$9f
    sta RAM_SEL+1
    sta ROM_SEL+1
    rts

:   lda #$01                    ;Version: other then R38, presume R39
    sta ROM_SEL
    lda #$00
    sta RAM_SEL
    sta ROM_SEL+1
    sta RAM_SEL+1
    rts

.endproc

;******************************************************************************
;Function name: main_greeting
;Purpose......: Displays a program greeting
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_greeting
    ldx #<ps
    ldy #>ps
    jsr ui_print
    rts

    ps: .byt 13,"*** basic loader 0.0.6 ***",13, "(c) 2021 stefan jakobsson",13,0
.endproc

;******************************************************************************
;Function name: main_get_sourcefile
;Purpose......: Prompts the user for a source file name to be loaded
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_get_sourcefile
    ;Prompt to load last file
    lda main_loadflag
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
    bra main_get_sourcefile     ;Invalid response, try again

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
msg_loadprev2: .byt 13, "load (cr=y/n): ",0

msg_prompt: .byt 13, "enter file name: ", 0

msg_loading: .byt   13, "loading...", 13, 0
msg_nofile: .byt 13, "no source file", 13, 0
.endproc

;******************************************************************************
;Function name: main_load
;Purpose......: Loads the source file
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_load
    ;Prepare pass 1
    jsr file_init
    jsr token_init
    jsr line_init
    jsr label_init

    ;Open source file
    jsr file_open
    cmp #0
    bne err1

    ;Read and process each line
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
    cmp #0
    bne err1
    
    ;Prepare pass 2
    jsr line_init

    ;Open source file
    jsr file_open

    ;Read and process each line again
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
    cmp #0
    bne err2

    ;We're done
    jsr file_close
    rts

.endproc

;******************************************************************************
;Function name: main_print_disk_status
;Purpose......: Displays disk status message
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_print_disk_status
    ;Check if there was a Kernal I/O error, i.e. before disk communication begun
    lda file_err
    beq :+

    ;Print I/O error message
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

    ;Else get and print status retrieved from the disk if there was an error
:   jsr file_status         ;Gets and stores disk status in file_buf
    cmp #0                  ;A = Status code
    beq :+                  ;0 => No error, exit without printing anything

    ldx #<(file_buf)        ;Print disk status
    ldy #>(file_buf)
    lda #%00000001
    jsr ui_msg

:   rts
.endproc

;******************************************************************************
;Function name: main_editor
;Purpose......: Loads and starts X16 Edit from the root folder of the SD card 
;               if present. Load path is "//:X16EDIT*.PRG". If the program
;               has been previously called upon to load a BASIC file, that
;               file will automatically be opened in the editor
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_editor
    ;Set file params
    lda #2
    ldx #8
    ldy #1
    jsr KERNAL_SETLFS

    ;Set X16 Edit executable path
    lda #path_end-path
    ldx #<path
    ldy #>path
    jsr KERNAL_SETNAM

    ;Load X16 Edit
    lda #0
    jsr KERNAL_LOAD

    ;Check for load errors
    bcs err

    ;Check if a BASIC file has been loaded previously
    lda main_loadflag
    bne :+
    jmp 2061            ;No, start the editor with an empty buffer

:   lda #<file_name     ;Yes, start the editor and open the previous BASIC file
    sta KERNAL_R0
    lda #>file_name
    sta KERNAL_R0+1
    lda file_len
    sta KERNAL_R1
    jmp 2064

err:
    ldx #<loaderrmsg
    ldy #>loaderrmsg
    jsr ui_print
    rts

path:
    .byt "//:x16edit*.prg"
path_end:

loaderrmsg: .byt 13,"x16 edit not found", 13, 0

.endproc

main_loadflag: .byt 0

.include "file.inc"
.include "ui.inc"
.include "line.inc"
.include "token.inc"
.include "label.inc"
.include "util.inc"
.include "msg.inc"
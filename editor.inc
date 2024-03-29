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
;Function name: editor_launch
;Purpose......: Starts the X16 Edit text editor. The function will first scan 
;               all ROM banks, and use a ROM based editor if present. If not
;               found in ROM, the function will try to load the editor from 
;               the root folder of the SD Card.  The Load path is "//:X16EDIT*.PRG". 
;               If the BASLOAD has been previously called upon to load a BASIC 
;               file, that file will automatically be opened in the editor
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc editor_launch
    ;Store current ROM bank on stack
    lda ROM_SEL
    pha

    ;Scan all ROM banks for X16 Edit; if present, there is an application signature at fff0
    stz ROM_SEL
    ldy #0

scanrom_loop:
    lda $fff0,y
    cmp editorsignature,y
    bne next_bank      ;Signature didn't match, check next ROM bank
    iny
    cpy #7
    bne scanrom_loop   ;7 matching chars is a complete signature match, if not, continue looking

    ;Start editor
    jsr set_editor_params
    jsr $c003
    
    pla                 ;Restore ROM bank to original value after returning from the editor
    sta ROM_SEL
    rts

next_bank:
    ldy #0
    inc ROM_SEL
    lda ROM_SEL
    cmp #0
    bne scanrom_loop    ;Fallthrough to loadfromfile if ROM bank wrapped around to 0


    ;Editor not found in ROM, try to load from SD card root folder
    ;Set file params
loadfromfile:
    ;Restore ROM bank to original value
    pla
    sta ROM_SEL
    
    ;Set logical file params
    lda #2
    ldx #8
    ldy #1
    jsr KERNAL_SETLFS

    ;Set executable path
    lda #path_end-path
    ldx #<path
    ldy #>path
    jsr KERNAL_SETNAM

    ;Load program from storage media
    lda #0
    jsr KERNAL_LOAD

    ;Check for load errors
    bcs err

    ;Start editor
    jsr set_editor_params
    jmp 2064

err:
    ldx #<loaderrmsg
    ldy #>loaderrmsg
    jsr ui_print
    rts

set_editor_params:
    ;Check if a BASIC file has been loaded previously
    lda loader_loadflag
    bne :+

    ;No previous file - set file name len = 0
    stz KERNAL_R1
    bra :++

    ;Set file name
:   lda #<file_name
    sta KERNAL_R0
    lda #>file_name
    sta KERNAL_R0+1
    lda file_len
    sta KERNAL_R1

    ;Set RAM banks used by the editor
:   ldx #(LABEL_BUCKET_COUNT/2+2)       ;RAM banks 1 to LABEL_BUCKET_COUNT/2+1 used by BASLOAD
    ldy #255
    rts

path:
    .byt "//:x16edit*.prg"
path_end:
loaderrmsg: .byt 13,"x16 edit not found", 13, 0
editorsignature: .byt $58,$31,$36,$45,$44,$49,$54
.endproc

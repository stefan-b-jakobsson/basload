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

.include "common.inc"
.include "appversion.inc"

;******************************************************************************
.segment "LCODE"
;Loaded at start of BASIC memory

;******************************************************************************
;Function name: main_setup
;Purpose......: Loads program into its final destination in RAM; also wedge
;               command setup
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_setup
    ;Set BASIC memory top
    sec
    jsr KERNAL_MEMTOP       ; Read current values, we need banked RAM top return in .A

    ldx #$00                ; Set BASIC memory top to $9000
    ldy #$90
    clc
    jsr KERNAL_MEMTOP

    ;Copy main program to address $9000-
    lda #<main_lcode_end
    sta TEMP1
    lda #>main_lcode_end
    sta TEMP1+1

    stz TEMP2
    lda #$90
    sta TEMP2+1

    lda #1
    sta $00

    ldy #0
    ldx #$0f
loop:
    lda (TEMP1),y
    sta (TEMP2),y
    iny
    bne loop
    inc TEMP1+1
    inc TEMP2+1
    dex
    bne loop

    ;Setup wedge command
    clc                     ;Fetch and store Kernal NGONE address from IGONE vector; also store NGONE1 address, which is NGONE+3
    
    lda BASIC_IGONE
    sta BASIC_NGONE
    adc #3
    sta BASIC_NGONE1
    
    lda BASIC_IGONE+1
    sta BASIC_NGONE+1
    adc #0
    sta BASIC_NGONE1+1

    lda #<main_wedge_parser ;Set address to custom wedge parser in IGONE vector
    sta BASIC_IGONE
    lda #>main_wedge_parser
    sta BASIC_IGONE+1

    ;Print program greeting
    ldx #0
:   lda msg,x
    beq exit
    jsr KERNAL_CHROUT
    inx
    bra :-

exit:
    rts

msg:
    .byt 13, .sprintf("*** basload %u.%u.%u ***", appversion_major, appversion_minor, appversion_patch), 13
    .byt "(c) 2021-2023, stefan jakobsson", 13, 13
    .byt "program copied to memory address $9000",13
    .byt "list of commands:",13
    .byt " !l or sys$9000 - load basic program from file",13
    .byt " !e or sys$9003 - start x16 edit",13,0
.endproc

main_lcode_end:

;******************************************************************************
;START OF CODE MOVED TO $9000 ON PROGRAM INIT
;******************************************************************************
.segment "CODE"

;******************************************************************************
;Jump table
    ;$9000
    jmp loader_prompt
    
    ;$9003
    jmp editor_launch

;******************************************************************************
;Function name: main_wedge_parser
;Purpose......: Wedge command parser. Supported commands are:
;               !l  => start basic loader (entry point $9000)
;               !e  => start X16 Edit (entry point $9003)
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc main_wedge_parser
    ;Check if start of wedge command
    jsr BASIC_CHRGET    ;Get next char
    
    cmp #'!'
    beq :+              ;Yes, continue
    jmp (BASIC_NGONE1)  ;No, exit

:   jsr BASIC_CHRGET    ;Get next char
    cmp #'l'            ;!l => basic loader
    beq load
    cmp #'e'            ;!e => X16 Edit
    beq editor
    
    jmp (BASIC_NGONE1)  ;Unkwon command, will result in ?SYNTAX ERROR

load:
    jsr loader_prompt
    jmp (BASIC_NGONE)

editor:
    jsr editor_launch
    jmp (BASIC_NGONE)

.endproc

BASIC_NGONE: .res 2
BASIC_NGONE1: .res 2

.include "file.inc"
.include "ui.inc"
.include "line.inc"
.include "token.inc"
.include "label.inc"
.include "util.inc"
.include "msg.inc"
.include "editor.inc"
.include "loader.inc"

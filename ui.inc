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
    ui_ptr: .res 2
.CODE

;******************************************************************************
;Function name: ui_print
;Purpose......: Prints a null terminated string
;Input........: Pointer to string, X = AddressL and Y = AddressH
;Output.......: Nothing
;Errors.......: Nothing
.proc ui_print
    stx ui_ptr
    sty ui_ptr+1

    ldy #0
:   lda (ui_ptr),y
    beq exit
    jsr KERNAL_CHROUT
    iny
    bne :-

exit:
    rts
.endproc

;******************************************************************************
;Function name: ui_msg
;Purpose......: Prints a program warning or error message
;Input........: Pointer to message, X = AddressL and Y = AddressH
;               A bits 0-3: Level, where 0000 is warning and 0001 is error
;               A bit    4: 1=show line number
;Output.......: Nothing
;Errors.......: Nothing
.proc ui_msg
    sta flags
    phx
    phy

    and #%00001111
    cmp #%00000001
    beq err

warn:
    ldx #<msg_warn
    ldy #>msg_warn
    jsr ui_print
    bra line

err:
    ldx #<msg_err
    ldy #>msg_err
    jsr ui_print

line:
    lda flags
    and #%00010000
    beq noline
    
    ldx #<msg_line
    ldy #>msg_line
    jsr ui_print
    
    ldx line_srclin
    ldy line_srclin+1
    lda line_srclin+2
    jsr util_bin_to_bcd
    jsr util_bcd_to_str
    jsr ui_print

    ldx #<msg_line2
    ldy #>msg_line2
    jsr ui_print
    bra msg

noline:
    ldx #<(msg_line2)
    ldy #>(msg_line2)
    jsr ui_print

msg:
    ply
    plx
    jsr ui_print

    lda #13
    jsr KERNAL_CHROUT
    
    rts
    
msg_warn:   .byt "warning",0
msg_err:    .byt "error",0
msg_line:   .byt " in line ",0
msg_line2:  .byt ": ",0

.segment "VARS"
    flags: .res 1
.CODE
.endproc
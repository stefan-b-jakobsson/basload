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

.segment "VARS"
    file_name: .res 256
    file_buf: .res 256
    file_len: .res 1
    file_device: .res 1
    file_err: .res 1
.CODE

;******************************************************************************
;Function name: file_init
;Purpose......: Initializes file functions
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc file_init
    lda #8
    sta file_device
    rts
.endproc

;******************************************************************************
;Function name: file_open
;Purpose......: Opens a source file for reading
;Input........: Nothing
;Output.......: Nothing
;Errors.......: A = error code or 0 if no error
.proc file_open
    stz file_err

    ;Ensure file #1 is closed
    lda #1
    jsr KERNAL_CLOSE

    ;Set file name
    lda file_len
    ldx #<file_name
    ldy #>file_name
    jsr KERNAL_SETNAM

    ;Set file params
    lda #1
    ldx file_device
    ldy #3
    jsr KERNAL_SETLFS

    ;Open
    jsr KERNAL_OPEN
    bcs err
    ldx #1
    jsr KERNAL_CHKIN

    lda #0
    rts

err:
    sta file_err
    rts
.endproc

;******************************************************************************
;Function name: file_close
;Purpose......: Closes source file
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc file_close
    lda #1
    jsr KERNAL_CLOSE
    jsr KERNAL_CHKIN
    rts
.endproc

;******************************************************************************
;Function name: file_readln
;Purpose......: Reads one line from the source file and stores it into a
;               buffer
;Input........: Nothing
;Output.......: A = 1 if EOF, A = 2 if file error, A = 3 line too long,
;               else A = 0
;Errors.......: Nothing
.proc file_readln
    stz index
    
loop:
    jsr KERNAL_CHRIN
    cmp #10
    beq eol
    cmp #13
    beq eol

    ldx index
    cpx #250
    bcc :+
    ldx #<msg_line_too_long
    ldy #>msg_line_too_long
    lda #%00010001
    jsr ui_msg
    lda #2
    rts

:   sta file_buf,x
    inc index

    jsr KERNAL_READST
    cmp #0
    beq loop
    cmp #64                 ;End of file, no errors
    beq eof
    
error:
    dec index
    
    lda #0
    ldx index
    sta file_buf,x

    lda #2
    rts

eof:    
    lda #0
    ldx index
    sta file_buf,x

    lda #1
    rts

eol:
    lda #0
    ldx index
    sta file_buf,x

    jsr KERNAL_READST
    cmp #0
    beq :+
    cmp #64
    beq eof
    bra error

:   lda #0
    rts

.segment "VARS"
    index: .res 1
.CODE
.endproc

;******************************************************************************
;Function name: file_status
;Purpose......: Reads disk status
;Input........: Nothing
;Output.......: A = status code in BCD format
;Errors.......: Nothing
.proc file_status
    stz file_err
    stz index

    lda #15
    jsr KERNAL_CLOSE

    lda #0
    jsr KERNAL_SETNAM

    lda #15
    ldx file_device
    ldy #15
    jsr KERNAL_SETLFS

    jsr KERNAL_OPEN
    bcs err

    ldx #15
    jsr KERNAL_CHKIN

loop:
    jsr KERNAL_CHRIN
    ldx index
    sta file_buf,x
    inc index

    jsr KERNAL_READST
    beq loop

    ldx index
    lda #0
    sta file_buf,x

    lda #15
    jsr KERNAL_CLOSE
    jsr KERNAL_CLRCHN

    sec
    lda file_buf
    sbc #48
    asl
    asl
    asl
    asl
    sta code

    sec
    lda file_buf+1
    sbc #48
    ora code
    sta code
    lda code
    rts

err:
    sta file_err
    rts

.segment "VARS"
    index: .res 1
    code: .res 1
.CODE
.endproc

file_ioerr_L:
    .byt <file_ioerr_1, <file_ioerr_2, <file_ioerr_3, <file_ioerr_4, <file_ioerr_5, <file_ioerr_6, <file_ioerr_7, <file_ioerr_8, <file_ioerr_9
file_ioerr_H:
    .byt >file_ioerr_1, >file_ioerr_2, >file_ioerr_3, >file_ioerr_4, >file_ioerr_5, >file_ioerr_6, >file_ioerr_7, >file_ioerr_8, >file_ioerr_9
file_ioerr_1:
    .byt "too many open files", 0
file_ioerr_2:
    .byt "file already open", 0
file_ioerr_3:
    .byt "file not open", 0
file_ioerr_4:
    .byt "file not found", 0
file_ioerr_5:
    .byt "device not present", 0
file_ioerr_6:
    .byt "file is not an input file", 0
file_ioerr_7:
    .byt "file is not an output file", 0
file_ioerr_8:
    .byt "file name is missing", 0
file_ioerr_9:
    .byt "illegal device number", 0
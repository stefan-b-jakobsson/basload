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

.segment "VARS"
    file_device: .res 1         ;Device number
    file_err: .res 1            ;I/O error code
    file_buf: .res 256          ;Input buffer
    prevchar: .res 1
.CODE

file_name: .res 256         ;File name
file_len: .res 1            ;Length of file name

;******************************************************************************
;Function name: file_init
;Purpose......: Initializes file functions
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc file_init
    ;Set device no
    lda #8
    sta file_device
    rts
.endproc

;******************************************************************************
;Function name: file_open
;Purpose......: Opens a source file for reading; before calling, store
;               file name in variable "file_name", file name length
;               in variable "file_len", and device number in
;               variable "file_device". The device defaults to 8 if
;               you have called the file_init function.
;Input........: Nothing
;Output.......: A = 0 if file open is successful
;Errors.......: A = error code (!= 0)
.proc file_open
    ;Clear error
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

    stz prevchar

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
;Purpose......: Reads one line from the source file and stores it in the
;               "file_buf" buffer
;Input........: Nothing
;Output.......: A = 0 if OK but not EOF
;               A = 1 if OK and EOF
;Errors.......: A = 2 if error reading line, call function "file_status" to
;               read the error code and message
.proc file_readln
    stz index
    stz quotes
    
loop:
    ;Read char from file
    jsr KERNAL_CHRIN
    ldx prevchar
    sta prevchar

    cmp #10                 ;LF
    bne :+
    cpx #13
    beq next
    bra eol

:   cmp #13                 ;CR
    beq eol

    ;Check line length
    ldx index
    cpx #250
    bcc :+
    ldx #<msg_line_too_long
    ldy #>msg_line_too_long
    lda #%00010001
    jsr ui_msg
    lda #2
    rts

:   ;Check quote
    cmp #34
    bne :+
    inc quotes
    bra store_char

:   ;Store char in Y register temporarily
    tay

    ;Convert to ASCII upper case if not within a string
    lda quotes
    and #%00000001
    bne :+                  ;Within string, store char as is

    cpy #97
    bcc :+                  ;Less than 97, store char as is
    cpy #123
    bcs :+                  ;Equal or greater than 123, store char as is

    tya                     ;Char is lower case, clear bit 5 to convert to ASCII upper case
    and #%11011111
    bra store_char

    ;Transfer char back to A
:   tya

    ;Store char in buffer
store_char:
    sta file_buf,x
    inc index

next:
    ;Read file status
    jsr KERNAL_READST
    cmp #0                  ;Status = 0 => Continue reading
    beq loop
    cmp #64                 ;Status = 64 => End of file without errors
    beq eof
    
error:                      ;Status != 0 && != 64 => An error
    dec index               ;Discard last char, wasn't valid
    lda #0
    ldx index
    sta file_buf,x

    lda #2                  ;Return code = 2 => Error
    rts

eof:    
    lda #0                  ;Set NULL at end of line
    ldx index
    sta file_buf,x

    lda #1                  ;Return code = 1 => EOF
    rts

eol:
    lda #0                  ;Set NULL at end of line
    ldx index
    sta file_buf,x

    jsr KERNAL_READST       ;Read file status
    cmp #0                  ;0 = OK
    beq :+
    cmp #64                 ;64 = EOF
    beq eof
    bra error               ;Other values => An error

:   lda #0                  ;Return code = 0 => OK, not EOF
    rts

.segment "VARS"
    index: .res 1
    quotes: .res 1
.CODE
.endproc

;******************************************************************************
;Function name: file_status
;Purpose......: Reads disk status
;Input........: Nothing
;Output.......: A = status code in BCD format
;               Complete status message stored in "file_buf" buffer
;Errors.......: Nothing
.proc file_status
    ;Init
    stz file_err
    stz index

    ;Ensure file #15 is closed
    lda #15
    jsr KERNAL_CLOSE

    ;Set empty file name, len=0, doesn't matter whereto X and Y points
    lda #0
    jsr KERNAL_SETNAM

    ;Set file params
    lda #15
    ldx file_device
    ldy #15
    jsr KERNAL_SETLFS

    ;Open file
    jsr KERNAL_OPEN
    bcs err

    ldx #15
    jsr KERNAL_CHKIN

loop:
    ;Read char
    jsr KERNAL_CHRIN
    ldx index
    sta file_buf,x
    inc index

    ;Read file status
    jsr KERNAL_READST
    beq loop

    ;Set NULL at end of buffer
    ldx index
    lda #0
    sta file_buf,x

    ;Close file
    lda #15
    jsr KERNAL_CLOSE
    jsr KERNAL_CLRCHN

    ;Format status code as BCD value
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

;******************************************************************************
;Function name: file_print_disk_status
;Purpose......: Displays disk status message
;Input........: Nothing
;Output.......: Nothing
;Errors.......: Nothing
.proc file_print_disk_status
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
;Kernal I/O error messages

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

# BASLOAD

## Introduction

BASLOAD is made in 65C02 assembly for the Commander X16 platform that was devised by David Murray, a.k.a. the 8-Bit Guy.

The purpose of BASLOAD is to enhance the programming experience by letting you write BASIC source code in the editor of your choosing without line numbers.

BASLOAD is best described as a BASIC tokenizer. It takes BASIC source code stored on disk in plain text format, and loads it into RAM. While loading
the file, it's tokenized so that it can be run by the built-in BASIC interpreter.

Even though the BASIC source code may be written in any editor, there are special bindings for the text editor X16 Edit in order to create an effective workflow.


## Hello World

As customary, let's first make a Hello World program.

```
LOOP:
    PRINT "HELLO, WORLD"
    GOTO LOOP
```

As may be seen, a label is defined on the first line, and called on the last line. No line numbers are used. Apart from that,
the BASIC code is the same as the standard X16 BASIC.


## Creating source files

The source files may be created by any means if the following requirements are met:

* The source file must be stored on disk (the X16's SD card)
* The content of the file must be plain PETSCII or ASCII encoded text
* Line breaks are encoded with CR and/or LF; any combination thereof should work
* Lines may not be longer than 250 characters
* The source file may not be more than 1,677,215 lines (24 bit counter)
* The resulting BASIC code may not be more than 65,535 lines (16 bit counter, a limitation of the platform)

Line numbers are not used or allowed in source files. Instead you define named labels as target for GOTO,  GOSUB and THEN commands.


## Labels

Labels are defined at the beginning of a line in the source file. Labels may be preceded by blank spaces, but no other characters.

The first character of a label must be within A-Z. The subsequent characters may also contain digits. No other characters are allowed in a label name.

A label definition is ended by a colon (':').

A label name may not be exactly the same as a reserved BASIC token.

Some examples:

* LOOP: is a valid label definition
* LOOP1: is a valid label definition
* 1LOOP: is not a valid label definition, first character must be A-Z
* PRINT: is not a valid label definition, is a reserved token
* PRINTME: is a valid label definition; if you actually want to PRINT the value of ME, and end it with a colon to start a new statement, you need to separate PRINT and ME with a blank space

The symbol table where the labels are stored is in banked RAM and may for now hold at most 2,048 items. The following information is kept for each label:

* At most the first twelve characters of the label name
* The length of the label name
* A checksum, calculated by adding the PETSCII values of the label name
* The line number in the tokenized BASIC code whereto the label points

It is possible to use labels longer than twelve characters, but that may cause "false" duplicates. BASLOAD will warn you about this, and if there is an actual duplicate definition (true or false), the program will stop with an error.

Labels are used after GOTO, GOSUB and THEN statements. To use a label, just type its name (without the colon).


## Load BASLOAD into memory

BASLOAD is loaded from the SD card as a normal BASIC program.

If stored on the SD card, load it with LOAD"BASLOAD-x.x.x.PRG",8 within the emulator.

If stored in the host file system, start the emulator with x16emu -prg BASLOAD-x.x.x.PRG -sdcard sdcard.img

Type RUN to setup BASLOAD. This will copy the program to its final destination in RAM ($9000), and also
setup the wedge commands that start with a ! character. A short description on how to use BASLOAD is displayed.


## Loading and tokenizing BASIC source files

Type the wedge command !L or SYS $9000 to start the main function of the program, that is loading a BASIC source file
into memory.

If the program has been run previously it will first ask if you want to reload the same source file as before.

Otherwise the program prompts you to enter the source file name.

BASLOAD outputs no message if the procedure is successful. If all goes well nothing is said, and you are only greeted with READY.

List of errors and warnings that may occur:

* LINE TOO LONG: if a line in the source file is longer than 250 characters
* DUPLICATE LABEL DEFINITION: if you try to define the same label name more than once
* LONG LABEL, MAY CAUSE FALSE DUPLICATES: a warning issued if a label definition is longer than twelve characters
* UNDEFINED OR MISSING LABEL AFTER GOTO OR GOSUB: raised if there is no label after a GOTO or GOSUB or if the label has not been defined.
* SYMBOL TABLE FULL: Raised if there is no more room in the symbol table for labels
* LINE NUMBER OVERFLOW: Raised if the tokenzied BASIC code is more than 65,535 lines


## Starting X16 Edit from BASLOAD

The text editor X16 Edit may be started from BASLOAD to make programming more convenient.

To do this, type the wedge command !E or SYS $9003. 

The program first searches for X16 Edit in the ROM banks.

If not found in ROM, the program will try to load X16 Edit from the SD card, and expects the program file to
be in the root folder. The CBM DOS path used to load X16 Edit is "//:X16EDIT*.PRG". If there are several versions
of X16 Edit stored in the root folder, the first one encountered will be loaded and started.

If BASLOAD already has been used to load a BASIC source file, that file will automatically be opened
in the text editor.


## BASLOAD + X16 Edit workflow

BASLOAD does not require you to use X16 Edit, but there are some benefits.

Using BASLOAD and X16 Edit in conjunction makes the following convenient workflow possible:

* Type !E or SYS $9003 to launch the text editor - if you already have used BASLOAD, the last BASIC source file will be opened automatically
* Edit the source file, save it to disk and quit the editor
* Type !L or SYS $9000 to load and tokenize the source file
* Type RUN to test the program
* Repeat


## Save BASIC programs in tokenized form

If you want to save a BASIC program in tokenized form, i.e. as a normal
BASIC program for the X16, just load the source file with BASLOAD and then
save the program with the standard SAVE command.
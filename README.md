# BASLOAD

## Introduction

BASLOAD is made in 65C02 assembly for the Commander X16 platform that was devised by David Murray, a.k.a. the 8-bit guy.

BASLOAD is best described as a BASIC tokenizer. It takes BASIC source code stored on disk in plain text format, and loads it into RAM. While loading
the file, it's tokenized so that it can be run by the built-in BASIC interpreter.

The purpose of BASLOAD is to enhance the programming experience by letting you edit BASIC source code in the editor of your choosing without using line numbers. Instead of
ine numbers, named labels are defined as targets for GOTO and GOSUB statements and after THEN statements.

## Creating source files

The source files may be created by any means if the following requirements are met:

* The source file must be stored on disk (the X16's SD card)
* The content of the file must be plain PETSCII encoded text
* Lines may not be longer than 250 characters
* The source file may not be more than 1,677,215 lines (24 bit counter)
* The resulting BASIC code may not be more than 65,535 lines (16 bit counter, a limitation of the platform)

Line numbers are not used in source files. Instead you define named labels as target for GOTO,  GOSUB and THEN statements.

Apart from that, the BASIC code is the same as when you program directly in the built-in BASIC editor on the X16.

## Defining labels

Labels are defined at the beginning of a line in the source file. It is, however, allowed to blank spaces before the start of a label definition.

The first character of a label must be within A-Z. The subsequent characters may also contain digits. No other characters are allowed in the label name.

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
* A line number in the tokenized BASIC code

It is possible to use labels longer than twelve characters, but that may cause "false" duplicates. BASLOAD will warn you about this, and if there is an actual duplicate definition (true or false), the
program will stop with an error.

## Using labels

Labels are used where line numbers normally are used, i.e. as targets for GOTO and GOSUB commands, and after THEN commands.

BASLOADS expects there to be a valid label reference after GOTO and GOSUB commands, and will stop with an error if there is not. After THEN commands, there may be a label, but also other statements.

To use a label, just type its name (without colon).

A label need not be separated from the preceding command by a blank space or any other delimiter. BASLOAD, however, expects that a label continues until encountering a character that is not allowed in label names (A-Z and digits).

## Source code example

Here follows a simple example illustrating the BASLOAD source code format:

```
LOOP:
    PRINT "HELLO, WORLD"
    GOTO LOOP
```

## Loading and running

BASLOAD resides in RAM at address $9000.

If stored on the SD card, load it with LOAD"BASLOAD.PRG",8,1 within the emulator.

If stored in the host file system, start the emulator with x16emu -prg BASLOAD.PRG,9000 -sdcard sdcard.img

You may then start BASLOAD with SYS $9000.

The program prompts you for the name of a source file. Just enter the name and press enter.

If no warnings or errors are displayed you should get the READY message. Type RUN to start the BASIC program you loaded. Or type SAVE "MYPROGRAM.PRG",8 to store it in tokenized form on disk.

## Tokenizer warnings and errors

BASLOAD is not chatting with you as a source file is loaded. If all goes well nothing is said, and you are greeted with READY.

While loading a source file you may get the following warnings and errors:

* LINE TOO LONG: if a line in the source file is longer than 250 characters
* DUPLICATE LABEL DEFINITION: if you try to define the same label name more than once
* LONG LABEL, MAY CAUSE FALSE DUPLICATES: a warning issued if a label definition is longer than twelve characters
* UNDEFINED OR MISSING LABEL AFTER GOTO OR GOSUB: raised if there is no label after a GOTO or GOSUB or if the label has not been defined.
* SYMBOL TABLE FULL: Raised if there is no more room in the symbol table for labels
* LINE NUMBER OVERFLOW: Raised if the tokenzied BASIC code is more than 65,535 lines
#!/bin/bash

cl65 -v -o BASLOAD.PRG -u __EXEHDR__ -Ln basload.lib -m basload.map -t cx16 -C cx16-asm.cfg main.asm

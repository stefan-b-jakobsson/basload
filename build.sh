#!/bin/bash

cl65 -v --asm-args -Dkernal_rev=39 -o BASLOAD.PRG -u __EXEHDR__ -m basload.map -t cx16 -C cx16-asm.cfg main.asm
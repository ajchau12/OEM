Some useful tips for finding ld files for future avr platforms:

Lookup Table from chip name to ldscript name:
https://github.com/embecosm/avr-gcc/blob/avr-gcc-mainline/gcc/config/avr/avr-mcus.def

If you have avr-gcc installed then navigate to /usr/lib/avr/lib/ldscripts/ and find the corresponding ldscript from the above lookup table
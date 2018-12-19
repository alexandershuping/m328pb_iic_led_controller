#!/usr/bin/env python3
''' Outputs random colors to 0x30 and 0x31 '''

import random
import serial
import time

def main():
    r = b'0%03x' % random.randrange(0, 0x1000)
    g = b'0%03x' % random.randrange(0, 0x1000)
    b = b'0%03x' % random.randrange(0, 0x1000)
    d = b'%02x' % random.randrange(0x30, 0x32)
    SER.write(b'xt' + d + b'1r' + r + b'g' + g + b'b' + b)


SER = serial.Serial('/dev/ttyUSB0', 38400, parity=serial.PARITY_NONE)
while True:
    for i in range(0, 500):
        main()

    # Explanation:
    #   x       - clear any previous command that might have been left
    #             unfinished
    #   t 00    - select general-call
    #   1       - put all devices into single-color (i.e. frozen) mode
    #   i 30 40 - include device 0x30 and indicate that it should have
    #             a phase offset of 0x40 (0x40 / 0xff == 25%)
    #   i 31 c0 - include device 0x31 and indicate that it should have
    #             a phase offset of 0xc0 (0xc0 / 0xff == 75%)
    #   S       - perform an exclusive-synchronize. 0x30 will go to 25%
    #             offset, 0x31 will go to 75% offset, any other devices
    #             will be unaffected
    #   2       - put all devices into sine-wave mode.
    #
    # 0x30 and 0x31 are now synchronized, with 0x31 leading 0x30 by 50%
    SER.write(b'xt001i3040i31c0S2')
    time.sleep(5)

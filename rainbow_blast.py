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
    # SER.write(b'xt001')
    # time.sleep(3)
    SER.write(b'xt001i3040i31c0S2')
    time.sleep(5)

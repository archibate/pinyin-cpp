with open('data/gbk.bin', 'wb') as f:
    for hi in range(0x81, 0xfe + 1):
        for lo in range(0x40, 0xfe + 1):
            n = hi << 8 | lo
            try:
                c = ord(n.to_bytes(2, 'big').decode('gbk'))
            except UnicodeDecodeError:
                c = 0xfffd
            f.write(c.to_bytes(2, 'little'))

with open('data/gbk-inv.bin', 'wb') as f:
    for c in range(0, 0xffff):
        s = chr(c)
        try:
            n = s.encode('gbk')
        except UnicodeEncodeError:
            n = b'\x00?'
        if len(n) > 1:
            assert len(n) == 2
        else:
            assert len(n) == 1
            n += b'\x00'
        f.write(n)

def untone_pinyin(pinyin, toned=True):
    if pinyin == 'ň':
        return 'en3'
    if pinyin == 'ňg':
        return 'eng3'
    if pinyin == 'm̄':
        return 'mun1'
    if pinyin == 'ḿ':
        return 'mun2'
    if pinyin == 'm̀':
        return 'mun4'
    if pinyin == 'ń':
        return 'en2'
    if pinyin == 'ńg':
        return 'eng2'
    if pinyin == 'ǹ':
        return 'en4'
    if pinyin == 'ǹg':
        return 'eng4'
    # e.g. xiǎo -> xiao3
    table = {
        'ā': 'a1',
        'á': 'a2',
        'ǎ': 'a3',
        'à': 'a4',
        'ō': 'o1',
        'ó': 'o2',
        'ǒ': 'o3',
        'ò': 'o4',
        'ē': 'e1',
        'é': 'e2',
        'ě': 'e3',
        'è': 'e4',
        'ê': 'e',
        'ê': 'e',
        'ế': 'e',
        'ề': 'e',
        'ī': 'i1',
        'í': 'i2',
        'ǐ': 'i3',
        'ì': 'i4',
        'ū': 'u1',
        'ú': 'u2',
        'ǔ': 'u3',
        'ù': 'u4',
        'ü': 'v',
        'ǖ': 'v1',
        'ǘ': 'v2',
        'ǚ': 'v3',
        'ǜ': 'v4',
    }
    res = ''
    tone = 0
    for c in pinyin:
        if c in table:
            e = table[c]
            if len(e) > 1:
                assert len(e) == 2
                assert e[1] in '1234', e[1]
                tone = int(e[1])
                e = e[0]
            res += e[0]
        elif ord(c) not in (0x304, 0x30c):
            assert c in 'abcdefghijklmnopqrstuvwxyz', (pinyin, c)
            res += c
    if tone > 0 and toned:
        res += str(tone)
    return res

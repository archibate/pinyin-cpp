import math
from pinyintools import untone_pinyin

tabPin = {}
tabFreq = {}
tab = {}

# wget https://lingua.mtsu.edu/chinese-computing/statistics/char/download.php?Which=TO -O build/chars.txt
with open('build/chars.txt', encoding='gb18030') as f:
    for line in f:
        if line.startswith('/*'):
            continue
        num, char, count, cdf, pinyin, english = line.split('\t')
        tabFreq[ord(char)] = int(count)

# wget https://raw.githubusercontent.com/mozillazg/pinyin-data/master/pinyin.txt -O build/pinyin.txt
with open('build/pinyin.txt', encoding='utf-8') as f:
    for line in f:
        if not line.startswith('U+'):
            continue
        unicode, pinyin = line.split(':')
        char = int(unicode[2:], base=16)
        pinyin = pinyin.split('#')[0].strip().split(',')
        tabPin[char] = list(map(untone_pinyin, pinyin))

for char, count in tabFreq.items():
    if char not in tab:
        tab[char] = [count, []]
    else:
        tab[char][0] += count

for char, pinyin in tabPin.items():
    if char not in tab:
        tab[char] = [0, pinyin]
    else:
        tab[char][1] = list(set(tab[char][1] + pinyin))

tabWord = {}

# wget https://raw.githubusercontent.com/mozillazg/phrase-pinyin-data/master/large_pinyin.txt -O build/large_pinyin.txt
with open('build/large_pinyin.txt', 'r', encoding='utf-8') as f:
    for line in f:
        line = line.strip()
        line = line.split('#')[0]
        line = line.strip()
        if not line:
            continue
        # line example: "商鞅: shāng yāng"
        word, pinyin = line.split(':')
        pinyin = tuple(untone_pinyin(p) for p in pinyin.split())
        tabWord[pinyin] = tabWord.get(pinyin, set()).union(set([word]))

tabPinyin = []
tabPids: dict[str, int] = {}

for char, line in tab.items():
    count, pinyins = line
    for pinyin in pinyins:
        if pinyin[-1].isdigit():
            pinyin = pinyin[:-1]
        if pinyin in tabPids:
            pid = tabPids[pinyin]
        else:
            pid = len(tabPinyin)
            tabPinyin.append(pinyin)
            tabPids[pinyin] = pid

print(len(tabPinyin))
print(len(tab))
print(len(tabWord))

with open('data/pinyin.bin', 'wb') as f:
    f.write(len(tabPinyin).to_bytes(4, 'little'))
    l = 0
    for pinyin in tabPinyin:
        data = pinyin.encode('ascii')
        l += len(data)
        data += b'\x00' * (6 - len(data))
        f.write(data)
    f.write(len(tab).to_bytes(4, 'little'))
    for char, line in tab.items():
        count, pinyins = line
        logProb = math.log(count + 1.1)
        logProbQuant = int(logProb * 4096)
        data = char.to_bytes(3, 'little')
        data += logProbQuant.to_bytes(2, 'little')
        data += len(pinyins).to_bytes(1, 'little')
        for pinyin in pinyins:
            tone = 0
            if pinyin[-1].isdigit():
                tone = int(pinyin[-1])
                pinyin = pinyin[:-1]
            pid = tabPids[pinyin] * 8 + tone
            data += pid.to_bytes(2, 'little')
        f.write(data)

with open('data/pinyin-words.bin', 'wb') as f:
    f.write(len(tabWord).to_bytes(4, 'little'))
    for pinyins, words in tabWord.items():
        data = len(pinyins).to_bytes(1, 'little')
        for pinyin in pinyins:
            tone = 0
            if pinyin[-1].isdigit():
                tone = int(pinyin[-1])
                pinyin = pinyin[:-1]
            pid = tabPids[pinyin] * 8 + tone
            data += pid.to_bytes(2, 'little')
        for word in words:
            word16 = word.encode('utf-16')
            data += (len(word16) // 2).to_bytes(1, 'little')
            data += word16
        data += b'\x00'
        f.write(data)

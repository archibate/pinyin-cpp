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
        tabFreq[ord(char)] = count

# wget https://raw.githubusercontent.com/mozillazg/pinyin-data/master/pinyin.txt -O build/pinyin.txt
with open('build/pinyin.txt', encoding='utf-8') as f:
    for line in f:
        if not line.startswith('U+'):
            continue
        unicode, pinyin = line.split(':')
        char = int(unicode[2:], base=16)
        pinyin = pinyin.split('#')[0].strip().split(',')
        tabPin[char] = ' '.join(map(untone_pinyin, pinyin))

for char, count in tabFreq.items():
    if char not in tab:
        tab[char] = [count, '']
    else:
        tab[char][0] += count

for char, pinyin in tabPin.items():
    if char not in tab:
        tab[char] = [0, pinyin]
    else:
        tab[char][1] = pinyin

with open('data/chars.csv', 'w', encoding='utf-8') as f:
    f.write('\n'.join(str(char) + ',' + str(line[0]) + ',' + line[1] for char, line in tab.items()) + '\n')

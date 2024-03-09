import pickle
from pinyintools import untone_pinyin

tab = {}

# wget https://github.com/System-T/DimSim/raw/master/dimsim/data/pinyin_to_simplified.pickle -O build/pinyin_to_simplified.pickle
# with open('build/pinyin_to_simplified.pickle', 'rb') as f:
#     for pinyin, word in pickle.load(f).items():
#         pinyin = pinyin.lower().split()
#         pinyin = tuple(p.upper() if len(p) == 1 else p[:-1] if p[-1] in '12345' else p for p in pinyin)
#         tab[pinyin] = tab.get(pinyin, set()).union(set(word))

# wget https://raw.githubusercontent.com/mozillazg/phrase-pinyin-data/master/large_pinyin.txt -O build/large_pinyin.txt
with open('build/large_pinyin.txt', 'r', encoding='utf-8') as f:
    for line in f:
        line = line.strip()
        line = line.split('#')[0]
        line = line.strip()
        if not line:
            continue
        # line example: 商鞅: shāng yāng
        word, pinyin = line.split(':')
        pinyin = tuple(untone_pinyin(p, toned=False) for p in pinyin.split())
        tab[pinyin] = tab.get(pinyin, set()).union(set([word]))

with open('data/words.csv', 'w', encoding='utf-8') as f:
    for pinyin, word in tab.items():
        f.write('{}\t{}\n'.format(' '.join(pinyin), ' '.join(word)))

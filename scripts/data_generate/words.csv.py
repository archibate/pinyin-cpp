import pickle

tab = {}

# pinyin_to_simplified.pickle can be obtained from: https://github.com/System-T/DimSim/raw/master/dimsim/data/pinyin_to_simplified.pickle
with open('build/pinyin_to_simplified.pickle', 'rb') as f:
    for pinyin, word in pickle.load(f).items():
        pinyin = pinyin.lower().split()
        pinyin = tuple(p.upper() if len(p) == 1 else p[:-1] if p[-1] in '12345' else p for p in pinyin)
        tab[pinyin] = tab.get(pinyin, set()).union(set(word))

with open('build/words.csv', 'w') as f:
    for pinyin, word in tab.items():
        f.write('{}\t{}\n'.format(' '.join(pinyin), ' '.join(word)))

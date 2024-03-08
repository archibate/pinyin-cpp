tab = []

# chars.txt can be obtained from: https://lingua.mtsu.edu/chinese-computing/statistics/char/download.php?Which=TO
with open('build/chars.txt') as f:
    for line in f:
        if line.startswith('/*'):
            continue
        num, char, count, cdf, pinyin, english = line.split('\t')
        tab.append((str(ord(char)), count, pinyin))

with open('build/chars.csv', 'w') as f:
    f.write('\n'.join(','.join(line) for line in tab) + '\n')

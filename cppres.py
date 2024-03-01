# bake an binary file into a cpp source file (with char array)
import argparse

parser = argparse.ArgumentParser(description='Bake a binary file into a cpp source file.')
parser.add_argument('file', help='The binary file to bake.')
parser.add_argument('--out', '-o', help='The output file.', required=False)
parser.add_argument('--name', '-n', help='The array name.', required=False)
parser.add_argument('--size-name', '-s', help='The array size variable name.', required=False)
parser.add_argument('--namespace', '-N', help='The array namespace.', required=False)
args = parser.parse_args()

if not args.out:
    args.out = args.file + '.cpp'

if not args.name:
    args.name = ''
    for c in args.file:
        if c.isalnum():
            args.name += c
        else:
            args.name += '_'

if not args.size_name:
    args.size_name = '{}_size'.format(args.name)

with open(args.file, 'rb') as f:
    content = f.read()
    with open(args.out, 'w') as fout:
        if args.namespace:
            fout.write('namespace {} {{\n'.format(args.namespace))
        fout.write('extern const unsigned long long {} = {};\n'.format(args.size_name, len(content)))
        fout.write('extern const char {}[] = {{\n'.format(args.name))
        for i in range(len(content)):
            c = content[i]
            if c >= 0x80:
                c = c - 0x100
            fout.write('{:d},'.format(c))
            if i % 16 == 15:
                fout.write('\n')
        fout.write('\n};')
        if args.namespace:
            fout.write('\n}')
        fout.write('\n')
print('// Baked C++ source generated into {}. To use, simply declare:'.format(args.out))
if args.namespace:
    print('namespace {} {{'.format(args.namespace))
print('extern const char {}[];'.format(args.name))
print('extern const unsigned long long {};'.format(args.size_name))
print('// To create a string: std::string({}, {});'.format(args.name, args.size_name))
print('// To create a vector: std::vector<char>({}, {});'.format(args.name, args.size_name))
print('// To create a stream: std::istringstream(std::string({}, {}));'.format(args.name, args.size_name))
if args.namespace:
    print('}')

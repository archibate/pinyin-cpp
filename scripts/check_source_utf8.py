import os

# walk current directory:
for root, dirs, files in os.walk("."):
    if root != '.' and (root.startswith('.') or root == 'build'):
        continue
    # loop through files:
    for file in files:
        # check if file is utf-8 encoded:
        try:
            # open file and read contents:
            with open(os.path.join(root, file), "rb") as f:
                binary = f.read()
                try:
                    content = binary.decode('ascii')
                    print('ASCII:', file)
                except UnicodeDecodeError as e:
                    # print(e.start, e.end, file)
                    content = binary.decode('utf-8')
                    # print(hex(ord(content[0])))
                    if not content.startswith('\ufeff'):
                        print(f'UTF-8 (at byte {e.start}) but no BOM:', file)
                    else:
                        print('UTF-8:', file)
        except UnicodeDecodeError as e:
            print(f'Cannot decode with UTF-8 (at byte {e.start}):', file)

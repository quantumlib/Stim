import os
import sys

if len(sys.argv) == 1:
    directory = os.getcwd()
else:
    directory = sys.argv[1]

for filename in os.listdir(directory):
    split_filename = filename.split('.')
    if len(split_filename) == 3 and split_filename[1] == 'new':
        old_filename = split_filename[0] + '.' + split_filename[-1]

        nf = os.path.join(directory, filename)
        of = os.path.join(directory, old_filename)

        if os.path.isfile(of):
            print(f"Removing {old_filename}")
            os.remove(of)
            print(f"Renaming {filename} to {old_filename}")
            os.rename(nf, of)
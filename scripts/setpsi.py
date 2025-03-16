import json
import sys

# if len(sys.argv) < 3:
#     sys.exit(1)

try:
    n = int(sys.argv[1])
    m = int(sys.argv[2])
    # e = int(sys.argv[3])
except ValueError:
    print("intput must be int")
    sys.exit(1)


with open('./configure.json', 'r') as file:
    data = json.load(file)

data['nParties'] = n 
data['setSize'] = m
# data['e'] = e

# set nParties and setSize
with open('./configure.json', 'w') as file:
    json.dump(data, file, indent=4)
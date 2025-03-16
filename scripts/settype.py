import json
import sys

# if len(sys.argv) < 2:
#     sys.exit(1)

try:
    e = int(sys.argv[1])
except ValueError:
    print("intput must be int")
    sys.exit(1)


with open('./configure.json', 'r') as file:
    data = json.load(file)

data['e'] = e

# set nParties and setSize
with open('./configure.json', 'w') as file:
    json.dump(data, file, indent=4)
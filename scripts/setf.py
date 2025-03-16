import sys
import json

def main():
    if len(sys.argv) != 4:
        print("Usage: python3 setf.py <n> <m> <comma_separated_values>")
        sys.exit(1)
    
    n = int(sys.argv[1])
    m = int(sys.argv[2])
    a_str = sys.argv[3]
    # e = sys.argv[4]
    
    a = [int(x) for x in a_str.split(',')]
    
    with open('configure.json', 'r') as f:
        config = json.load(f)
    
    config['nParties'] = n
    for i in range(len(a)):
        config[f'col{i}'] = a[i]
    config['cols'] = sum(a)
    config['setSize'] = m

    with open('configure.json', 'w') as f:
        json.dump(config, f, indent=4)
    
    print(f"Updated configure.json with n={n} , m={m} and a={a}")

if __name__ == "__main__":
    main()

import json
import argparse

config_file = './configure.json'

def update_config(file_path, new_params):
    try:
        with open(file_path, 'r') as f:
            config = json.load(f)
        
        config.update(new_params)
        config['e'] = 51
        
        with open(file_path, 'w') as f:
            json.dump(config, f, indent=4)
        
        print("Update:")
        for key, value in new_params.items():
            print(f"{key}: {value}")
    
    except Exception as e:
        print(f"Error: {e}")

def main():
    parser = argparse.ArgumentParser(description="Update configure.json with new parameters")
    parser.add_argument('--nParties', type=int, required=True, help='Number of parties')
    parser.add_argument('--srow', type=int, required=True, help='Number of rows')
    parser.add_argument('--scol', type=int, required=True, help='Number of columns')

    args = parser.parse_args()

    new_config = {
        "nParties": args.nParties,
        "srow": args.srow,
        "scol": args.scol,
        "e":5
    }

    update_config(config_file, new_config)

if __name__ == '__main__':
    main()

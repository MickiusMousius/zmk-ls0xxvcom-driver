import os, re

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Find closing braces at the start of a line (end of functions/structs)
    # and ensure they are followed by exactly 3 newlines (2 blank lines).
    content = re.sub(r'(?m)^}(\s*)\n+', r'}\1\n\n\n', content)
    content = re.sub(r'(?m)^};(\s*)\n+', r'};\1\n\n\n', content)
    
    with open(filepath, 'w') as f:
        f.write(content)

# We want to process src and include directories, relative to module root
module_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
base_dirs = ['src', 'include']

for bdir in base_dirs:
    full_dir = os.path.join(module_dir, bdir)
    if os.path.exists(full_dir):
        for root, _, files in os.walk(full_dir):
            for file in files:
                if file.endswith('.c') or file.endswith('.h'):
                    process_file(os.path.join(root, file))

print("Added 2 blank lines after top-level functions and structs!")

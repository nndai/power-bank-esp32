import gzip
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
input_file = os.path.join(script_dir, "../pages/index.html")
output_name = "page_index"
output_header_filename = "page_index.h"
destination_dir = os.path.join(script_dir, "../pages")

# Gzip content
with open(input_file, 'rb') as f_in:
    content = f_in.read()
    compressed_content = gzip.compress(content)

# Generate Hex Dump (xxd -i style)
hex_array = []
for byte in compressed_content:
    hex_array.append(f"0x{byte:02x}")

# Format as C array (wrap every 12 bytes)
lines = []
for i in range(0, len(hex_array), 12):
    lines.append(", ".join(hex_array[i:i+12]))
c_array_content = ",\n    ".join(lines)

header_content = f"""#ifndef PAGE_INDEX_H
#define PAGE_INDEX_H

// This file was generated using python script
unsigned char page_index[] = {{
    {c_array_content}
}};
unsigned int page_index_len = {len(compressed_content)};

#endif
"""

# Write to file
with open(output_header_filename, "w") as f_out:
    f_out.write(header_content)

print(f"Generated {output_header_filename}")

# Move file
dest_path = os.path.join(destination_dir, output_header_filename)
# Remove strict move, just write to dist or replace
if os.path.exists(dest_path):
    os.remove(dest_path)
os.rename(output_header_filename, dest_path)
print(f"Moved to {dest_path}")

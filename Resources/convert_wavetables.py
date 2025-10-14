import os
import re
import struct

# ✅ Change this to the root folder containing your wavetable subfolders
ROOT_DIR = r"Wavetables"  # Modify if needed

# ✅ Regex to extract everything between { ... }
ARRAY_REGEX = re.compile(r'\{([^}]*)\}', re.DOTALL)

def extract_samples_from_h_file(file_path):
    """Parse a .h file and return a list of 256 int-like samples."""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    match = ARRAY_REGEX.search(content)
    if not match:
        print(f"  ⚠️  No array data found in {file_path}")
        return None

    data_block = match.group(1)
    tokens = data_block.replace('\n', ' ').replace('\r', ' ').split(',')

    samples = []
    for t in tokens:
        t = t.strip()
        if t == '':
            continue
        try:
            samples.append(int(t))
        except ValueError:
            print(f"  ⚠️  Skipping invalid token in {file_path}: {t}")

    if len(samples) != 256:
        print(f"  ⚠️  {file_path} has {len(samples)} samples (expected 256)")
        return None

    return samples


def main():
    for folder, subdirs, files in os.walk(ROOT_DIR):
        # ✅ Collect all .h files in this folder
        h_files = [f for f in files if f.lower().endswith('.h')]
        if not h_files:
            continue

        h_files.sort()  # stable ordering

        print(f"\nProcessing folder: {folder}")
        print(f"  Found {len(h_files)} .h files")

        for index, hname in enumerate(h_files):
            h_path = os.path.join(folder, hname)
            samples = extract_samples_from_h_file(h_path)

            if samples is None:
                continue

            # ✅ Create binary filename: 00.bin, 01.bin, 02.bin, ...
            bin_filename = f"{index:02d}.bin"
            bin_path = os.path.join(folder, bin_filename)

            with open(bin_path, 'wb') as bf:
                for s in samples:
                    # ✅ Wrap like C int16_t overflow
                    s_wrapped = ((s + 32768) % 65536) - 32768
                    bf.write(struct.pack('<h', s_wrapped))  # little-endian int16

            print(f"  ✅ {hname} → {bin_filename}")


if __name__ == "__main__":
    main()
    print("\n✅ Conversion complete!")

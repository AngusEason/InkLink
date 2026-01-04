from PIL import Image
import sys
import math

OUTPUT_WIDTH  = 800
OUTPUT_HEIGHT = 480
OUTPUT_BYTES  = OUTPUT_WIDTH * OUTPUT_HEIGHT // 4

# Define the 4 allowed colours (RGB) and their 2-bit codes
PALETTE = [
    ((0,   0,   0),   0b00),  # Black
    ((255, 255, 255), 0b01),  # White
    ((255, 201, 14),   0b10),  # Yellow
    ((255, 0,   0),   0b11),  # Red
]

def nearest_colour(rgb):
    r, g, b = rgb
    best_code = 0
    best_dist = float("inf")

    for (pr, pg, pb), code in PALETTE:
        dist = (r - pr)**2 + (g - pg)**2 + (b - pb)**2
        if dist < best_dist:
            best_dist = dist
            best_code = code

    return best_code

def image_to_4color_bytes(img):
    img = img.convert("RGB")
    pixels = list(img.getdata())

    out = bytearray()
    byte = 0
    count = 0

    for p in pixels:
        code = nearest_colour(p)
        shift = (3 - (count % 4)) * 2
        byte |= (code << shift)
        count += 1

        if count % 4 == 0:
            out.append(byte)
            byte = 0

    return out

def write_cpp(data, filename="../myimagedata.cpp"):
    with open(filename, "w") as f:
        f.write('#include "myimagedata.h"\n\n')
        f.write('// 4 Color Image Data 800*480\n')
        f.write('const unsigned char myImage4color[96000] = {\n')

        for i, b in enumerate(data):
            if i % 12 == 0:
                f.write("    ")
            f.write(f"0x{b:02X}, ")
            if i % 12 == 11:
                f.write("\n")

        f.write("\n};\n")

def main():
    img = Image.open("input.png")
    img = img.resize((OUTPUT_WIDTH, OUTPUT_HEIGHT), Image.LANCZOS)

    data = image_to_4color_bytes(img)
    # Convert 4-color bytes back to image and save
    img_4color = Image.new("RGB", (OUTPUT_WIDTH, OUTPUT_HEIGHT))
    pixels_4color = []
    for byte in data:
        for i in range(4):
            shift = (3 - i) * 2
            code = (byte >> shift) & 0b11
            rgb = PALETTE[code][0]
            pixels_4color.append(rgb)
    img_4color.putdata(pixels_4color)
    img_4color.save("colour_processed.png")

    if len(data) != OUTPUT_BYTES:
        raise RuntimeError("Output size mismatch")

    write_cpp(data)
    print("Generated myimagedata.cpp")

if __name__ == "__main__":
    main()

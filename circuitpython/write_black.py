import time
import displayio
import waveshare7in5bv3

# Minimal script to write a full black screen to the Waveshare 7.5" (H) display

displayio.release_displays()

# Initialize the display using the existing driver in this repo
display = waveshare7in5bv3.Waveshare7in5Bv3()

# Create a bitmap and palette that exactly match the display
WIDTH = 800
HEIGHT = 480
bitmap = displayio.Bitmap(WIDTH, HEIGHT, 4)
palette = displayio.Palette(4)

# Ensure palette entries map to black so every index renders as black
palette[0] = 0x000000
palette[1] = 0x000000
palette[2] = 0x000000
palette[3] = 0x000000

# Attach to display
group = displayio.Group()
tile_grid = displayio.TileGrid(bitmap, pixel_shader=palette)
group.append(tile_grid)
display.root_group = group

# Fill the bitmap with index 0 (black)
for y in range(HEIGHT):
    for x in range(WIDTH):
        bitmap[x, y] = 0

# Small pause then refresh the e-paper (may take a few seconds)
time.sleep(0.1)
display.refresh()

print("Wrote full black screen")

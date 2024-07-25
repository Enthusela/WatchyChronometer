import numpy as np
from PIL import Image
import sys

# Copy your array from https://javl.github.io/image2cpp/ here
# epd_bitmap_moonWax3qrt_02_000ccw     
source_array = [
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x01, 0xff, 0xff, 0x80, 
	0x03, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xe0, 0x0f, 0xff, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0xf8, 
	0x1f, 0xff, 0xff, 0xf8, 0x3f, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff, 0xfc, 0x7f, 0xff, 0xff, 0xfe, 
	0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 
	0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfe, 
	0x7f, 0xff, 0xff, 0xfe, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
]


def main(start_angle, end_angle, step_angle, bitmap_name="bmp"):
    char_arrays = {}
    for i, angle in enumerate(range(start_angle, end_angle + step_angle, step_angle)):
        char_array_name = f'{bitmap_name}_{i:03}_{abs(angle):003}'
        
        if (angle >= 0):
            char_array_name += 'ccw'
        else:
            char_array_name += 'cw'
        
        char_arrays[char_array_name] = rotate_char_array(source_array, angle, char_array_name)

    header_file = get_char_array_header_file(char_arrays, bitmap_name)

    with open(f'{bitmap_name}.h', '+w') as f:
        f.write(header_file)


def rotate_char_array(char_array, angle, rotated_char_array_name):
    binary_array = char_array_to_binary_array(char_array)
    image = Image.fromarray(binary_array)
    rotated_image = image.rotate(angle)
    rotated_binary_array = np.array(rotated_image) // 255
    rotated_char_array = binary_image_to_char_array(rotated_binary_array)
    rotated_char_array_string = char_array_to_cpp_string(rotated_char_array, rotated_char_array_name)
    
    # image.show()
    # rotated_image.show(title=rotated_char_array_name)
    # print(rotated_hex_array)
    
    # test_rotated_binary_image = hex_to_binary_image(rotated_hex_array)
    # test_rotated_image = Image.fromarray(test_rotated_binary_image)
    # test_rotated_image.show()

    return rotated_char_array_string


def char_array_to_binary_array(hex_array):
    # Convert hex array into binary image
    image_rows = 32
    image_cols = 32
    binary_image = np.zeros((image_rows, image_cols), dtype=np.uint8)
    for i, value in enumerate(hex_array):
        for j in range(8):
            pixel_num = (i * 8 + j)
            pixel_row = pixel_num // image_rows
            pixel_col = pixel_num % image_cols
            pixel_set = min(value & (1 << (7 - j)), 1) * 255
            binary_image[pixel_row, pixel_col] = pixel_set
            # print(f'i: {i : >3}, value: {value : >3}, j: {j}, pixel_num = {pixel_num : >4}, pixel_row = {pixel_row : >2}, pixel_col = {pixel_col : >2}, pixel_set: {pixel_set}')
    
    return binary_image


def binary_image_to_char_array(binary_image):
    rotated_hex_array = []
    for row in binary_image:
        for i in range(0, len(row), 8):
            byte = 0x00
            for j in range(8):
                if i + j < len(row) and row[i + j]:
                    byte |= 1 << (7 - j)
            rotated_hex_array.append(byte)
    
    return rotated_hex_array


def char_array_to_cpp_string(hex_array, char_array_name):
    bytes_per_row = 4 # set to your_image_width / 8, or to taste
    char_array = f'const unsigned char {char_array_name} [] PROGMEM = {{'
    for i, byte in enumerate(hex_array):
        if i % bytes_per_row == 0 or i == len(hex_array):
            char_array += '\n\t'
        char_array += f'0x{byte:02x}'
        if i < len(hex_array) - 1:
            char_array += ', '
    char_array += '\n};\n'

    return char_array


def get_char_array_header_file(char_arrays, bitmap_name):
    # Generate header file including array of char arrays
    header_file = f'#ifndef {bitmap_name.upper()}_BITMAPS_H\n'
    header_file += f'#define {bitmap_name.upper()}_BITMAPS_H\n\n'
    
    for array in char_arrays.values():
        header_file += array
    
    # Assuming output arrays are the same size as the source arrays
    header_file += f'// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = {len(source_array) * len(char_arrays)})'
    header_file += f'\nconst uint8_t {bitmap_name}_array_len = {len(char_arrays)};\n'
    header_file += f'const uint8_t {bitmap_name}_array_max_index = {len(char_arrays) - 1};\n'
    
    header_file += f'const unsigned char* {bitmap_name}_array[{len(char_arrays)}] = {{\n\t'
    for i, array_name in enumerate(char_arrays.keys()):
        header_file += array_name
        if i < len(char_arrays) - 1:
            header_file += ',\n\t'

    header_file += '\n};\n\n'
    header_file += f'#endif'

    return header_file


if __name__ == "__main__":
    try:
        main(int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), sys.argv[4])
    except IndexError:
        print('Usage: rotate_bitmaps.py start_angle end_angle step_angle char_array_name')


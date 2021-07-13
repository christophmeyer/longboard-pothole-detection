import argparse
import os

from PIL import Image
import numpy as np


def read_grayscale(input_file, width):
    """
    Reads the raw bytes from the input_file and returns a 2d numpy array of np.int8 pixel values.
    """
    with open(input_file, 'rb') as file:
        raw_bytes = file.read()
    flat_integers = [int(byte) for byte in raw_bytes]

    int_array = np.array([flat_integers[n*width:(n+1)*width]
                         for n in range(len(flat_integers)//width)], dtype=np.uint8)
    return int_array


def grayscale_to_jpg(input_file, output_dir, width):
    """
    Reads grayscale image, converts it to jpg and writes it back into the output dir. 
    """
    int_array = read_grayscale(input_file, width)
    image = Image.fromarray(int_array)
    image.save(os.path.join(output_dir, os.path.splitext(os.path.basename(input_file))[0]+'.jpg'))


def grayscale_dir_to_jpg(input_dir, output_dir, width, grayscale_extension='gs'):
    """
    Converts all images in the input_dir with the grayscale_extension to jpg in the output_dir. 
    """
    print('converting grayscale images in {} to jpg'.format(input_dir))
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for file_name in os.listdir(input_dir):
        if os.path.splitext(file_name)[-1] != ('.' + grayscale_extension):
            continue

        grayscale_to_jpg(input_file=os.path.join(input_dir, file_name),
                         output_dir=output_dir,
                         width=width)
    print('jpg images written to {}'.format(output_dir))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_dir')
    parser.add_argument('--output_dir')
    args = parser.parse_args()

    grayscale_dir_to_jpg(args.input_dir, args.output_dir, width=96, grayscale_extension='gs')

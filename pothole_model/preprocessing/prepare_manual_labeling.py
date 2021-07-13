import os
import argparse

import pandas as pd

from model.train import ModelConfig
from preprocessing.convert_images import grayscale_dir_to_jpg


def prepare_manual_labeling(config):
    """
    Loops over all capture directories in the raw_data_dir and prepares them for manual labeling.
    """
    for filename in os.listdir(config.raw_data_dir):
        current_dir = os.path.join(config.raw_data_dir, filename)
        if os.path.isdir(current_dir):
            prepare_capture_dir(current_dir, config)


def prepare_capture_dir(capture_dir, config, grayscale_extension='gs'):
    """ 
    Prepares a capture_dir for manual labeling. In particular, a labels_template.csv is created in
    each capture directory and the grayscale images are converted to jpg and in the
    converted_images_dir. 
    """
    print('preparing label template for {}'.format(capture_dir))
    list_of_image_files = []
    for file_name in os.listdir(capture_dir):
        if os.path.splitext(file_name)[-1] == ('.' + grayscale_extension):
            list_of_image_files.append(file_name)
    label_template = pd.DataFrame({'file': sorted(list_of_image_files)})
    label_template['label'] = 0
    label_template.to_csv(
        path_or_buf=os.path.join(capture_dir, 'labels_template.csv'),
        sep=';', header=True, index=False)
    grayscale_dir_to_jpg(input_dir=capture_dir, output_dir=os.path.join(
        config.converted_images_dir, os.path.split(capture_dir)[-1]), width=96)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config_path')
    args = parser.parse_args()

    config = ModelConfig(config_path=args.config_path)

    prepare_manual_labeling(config)

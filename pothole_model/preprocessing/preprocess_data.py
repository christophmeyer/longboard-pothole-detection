import argparse
import pandas as pd
import numpy as np
import os
from shutil import copyfile
from preprocessing.convert_images import read_grayscale
from sklearn.model_selection import train_test_split
from model.train import ModelConfig


def read_annotated_capture_data(input_dir):
    """
    Loops over all capture directories in the input_dir and puts them together. Assumes there
    to be a labels.csv. Only used the images that are in the labels.csv, ignores the others. Thus, 
    in order to remove images from the dataset, just delete the corresponding row in labels.csv.
    """
    picture_filenames = []
    labels_list = []
    for capture_dir in os.listdir(input_dir):
        annotated_labels = pd.read_csv(os.path.join(input_dir, capture_dir, 'labels.csv'), sep=';')
        new_filenames = [os.path.join(capture_dir, filename)
                         for filename in annotated_labels['file'].tolist()]
        picture_filenames += new_filenames
        new_labels = np.zeros(len(new_filenames))
        for id, label in enumerate(annotated_labels['label'].tolist()):
            new_labels[id] = label
        labels_list.append(new_labels)

    labels = np.concatenate(labels_list)
    return picture_filenames, labels


def save_data(input_dir, picture_filenames, labels, prefix, output_dir, augment=False):
    """
    Saves the data for a given split (train/val/test) to the subdirectory prefix of the output_dir.
    If augment=True, then also vertically flipped images are saved together with the correcponding
    labels.
    """

    print('Saving {} data to {}'.format(prefix, os.path.join(output_dir, prefix)))
    out_subdir = os.path.join(output_dir, prefix, 'features')
    if not os.path.exists(out_subdir):
        os.makedirs(out_subdir)
    if augment:
        labels = np.concatenate([labels, labels])
    np.savetxt(os.path.join(output_dir, prefix, 'labels.csv'), labels, delimiter=';')

    idx = 0
    for filename in picture_filenames:
        copyfile(os.path.join(input_dir, filename), os.path.join(
            out_subdir, '{}_{}.gs'.format(prefix, str(idx).zfill(6))))
        idx += 1

    if augment:
        for filename in picture_filenames:
            gs_img = read_grayscale(os.path.join(input_dir, filename), width=96)
            np.flip(
                gs_img, (1)).tofile(
                os.path.join(out_subdir, '{}_{}.gs'.format(prefix, str(idx).zfill(6))))
            idx += 1


def preprocess_data(config):
    """
    Combines that data from all capture directories in the raw_data_dir, splits the data into 
    train, validation and test and augments the data by vertically flipping images. The resulting
    data is saved into train, val, test subdirs of the train_data_dir.  
    """
    picture_filenames, labels = read_annotated_capture_data(config.raw_data_dir)

    X_train_tmp, X_test, y_train_tmp, y_test = train_test_split(
        picture_filenames, labels, test_size=config.test_split, random_state=1)
    X_train, X_val, y_train, y_val = train_test_split(
        X_train_tmp, y_train_tmp, test_size=config.validation_split, random_state=1)

    save_data(config.raw_data_dir, X_train, y_train, 'train',
              config.train_data_dir, config.augment_train_data)
    save_data(config.raw_data_dir, X_val, y_val, 'val',
              config.train_data_dir, config.augment_val_data)
    save_data(config.raw_data_dir, X_test, y_test, 'test',
              config.train_data_dir, config.augment_test_data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config_path')
    args = parser.parse_args()

    config = ModelConfig(config_path=args.config_path)

    preprocess_data(config)

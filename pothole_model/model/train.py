import os
import argparse

import numpy as np
from numpy import genfromtxt
import yaml
import tensorflow as tf

from preprocessing.convert_images import read_grayscale
from model.model import build_model


class ModelConfig():
    """
    Simple config class that reads properties from file on construction and keeps
    them as class attributes. 
    """

    def __init__(self, config_path):
        with open(config_path) as file:
            config_dict = yaml.load(file, Loader=yaml.FullLoader)
        self.__dict__ = config_dict


def load_dataset(data_path, batch_size, repeat=False):
    """
    Loads shuffles and batches a dataset and returns a tf.data.Dataset. The data is expected
    to be in a directory that containa labels.csv and a subdirectory features that holds the 
    grayscale images. 
    """
    list_of_files = sorted(os.listdir(os.path.join(data_path, 'features')))

    image_arrays = [read_grayscale(os.path.join(data_path, 'features', input_file),
                                   width=96).reshape(96, 96, 1) for input_file in list_of_files]
    images = np.stack(image_arrays, axis=0)

    raw_labels = genfromtxt(os.path.join(data_path, 'labels.csv'), delimiter=';')
    assert not np.any(np.isnan(raw_labels))

    one_hot_labels = tf.one_hot(raw_labels, depth=2)
    dataset = tf.data.Dataset.from_tensor_slices((tf.cast(images-128, tf.int8), one_hot_labels))

    if repeat:
        dataset = dataset.repeat()
    return dataset.shuffle(10000).batch(batch_size), len(list_of_files)


def run_model_training(config):
    """
    Reads training data, trains the models as specified in 
    config and saves it at model_save_path as SavedModel.
    """

    dataset_train, num_examples = load_dataset(
        data_path=os.path.join(config.train_data_dir, 'train'),
        batch_size=config.batch_size, repeat=True)
    dataset_val, _ = load_dataset(
        data_path=os.path.join(config.train_data_dir, 'val'),
        batch_size=config.batch_size, repeat=False)
    dataset_test, _ = load_dataset(
        data_path=os.path.join(config.train_data_dir, 'test'),
        batch_size=config.batch_size, repeat=False)

    steps_per_epoch = num_examples // config.batch_size
    total_train_steps = steps_per_epoch * config.epochs

    lr_schedule = tf.keras.optimizers.schedules.PolynomialDecay(
        initial_learning_rate=config.initial_learning_rate,
        decay_steps=total_train_steps,
        end_learning_rate=config.final_learning_rate,
        power=1.0,
        cycle=False,
        name=None
    )

    optimizer = tf.keras.optimizers.Adam(
        learning_rate=lr_schedule, beta_1=0.9, beta_2=0.999, epsilon=1e-07, amsgrad=False,
        name='Adam')

    model = build_model(config, optimizer)
    model.summary()

    print("Steps per epoch {}".format(steps_per_epoch))
    model.fit(dataset_train,
              batch_size=config.batch_size,
              steps_per_epoch=steps_per_epoch,
              validation_data=dataset_val,
              epochs=config.epochs)

    print('Evaluating on test dataset:')
    model.evaluate(dataset_test)

    print('Saving model to {}'.format(config.model_save_path))
    model.save(config.model_save_path)

    return


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config_path')
    args = parser.parse_args()

    config = ModelConfig(config_path=args.config_path)

    run_model_training(config)

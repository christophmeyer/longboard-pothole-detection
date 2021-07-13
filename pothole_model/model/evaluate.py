import os
import argparse

import tensorflow as tf
import numpy as np

from model.train import ModelConfig, load_dataset


def evaluate(interpreter, dataset):
    """
    Calculates the accuracy of the tflite model on the given dataset. 
    """
    interpreter.allocate_tensors()

    input_idx = interpreter.get_input_details()[0]["index"]
    output_idx = interpreter.get_output_details()[0]["index"]

    n_correct = 0.0
    n_total = 0.0

    print('Evaluating tflite model on test dataset...')
    for features, label in dataset:
        n_total += 1
        interpreter.set_tensor(input_idx, features)

        interpreter.invoke()

        probability = interpreter.get_tensor(output_idx)
        prediction = np.argmax(probability[0])

        if prediction == np.argmax(label):
            n_correct += 1

    return n_correct / n_total


def load_and_evaluate_tflite_model(config):
    """
    Loads the tflite model and the test dataset and calculates the accuracy of the model.
    """
    interpreter = tf.lite.Interpreter(model_path=config.tflite_model_path)

    dataset_test, _ = load_dataset(data_path=os.path.join(config.train_data_dir, 'test'),
                                   batch_size=1, repeat=False)

    accuracy = evaluate(interpreter, dataset_test)
    print("Accuracy: {}".format(accuracy))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config_path')
    args = parser.parse_args()

    config = ModelConfig(config_path=args.config_path)
    load_and_evaluate_tflite_model(config)

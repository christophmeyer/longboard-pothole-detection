import os

import tensorflow as tf

from model.train import load_dataset, ModelConfig


def convert_to_tflite(config):
    """
    Converts the SavedModel to a tflite model representation. Also perform the 8-bit integer
    quantization.
    """
    converter = tf.lite.TFLiteConverter.from_saved_model(config.model_save_path)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]

    dataset, _ = load_dataset(
        data_path=os.path.join(config.train_data_dir, 'train'),
        batch_size=config.batch_size)

    def representative_dataset():
        for sample in dataset.take(1):
            yield [tf.cast(sample[0], tf.float32)]

    converter.allow_custom_ops = False
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    converter.representative_dataset = representative_dataset

    model_quantized_tflite = converter.convert()

    print('Writing tflite model to {}'.format(config.tflite_model_path))
    with open(config.tflite_model_path, 'wb') as file:
        file.write(model_quantized_tflite)


if __name__ == '__main__':

    config = ModelConfig(config_path='./model/config.yaml')
    convert_to_tflite(config)

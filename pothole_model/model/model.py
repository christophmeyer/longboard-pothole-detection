import tensorflow as tf
from tensorflow.keras import layers


class DepthwiseConvBlock(layers.Layer):
    """
    Implements the depthwise convolution blocks used in http://arxiv.org/abs/1704.04861.
    """

    def __init__(self, pointwise_conv_filters, alpha, depth_multiplier=1, strides=(1, 1)):
        super(DepthwiseConvBlock, self).__init__()
        self.strides = strides
        pointwise_conv_filters = int(alpha*pointwise_conv_filters)

        if strides != (1, 1):
            self.zero_padding_2d = layers.ZeroPadding2D(((0, 1), (0, 1)))

        self.depthwise_conv_layer = layers.DepthwiseConv2D((3, 3), padding='same' if strides == (
            1, 1) else 'valid', depth_multiplier=depth_multiplier, strides=strides, use_bias=False)

        self.batch_norm = layers.BatchNormalization()
        self.relu = layers.ReLU(max_value=6)

        self.conv2d = layers.Conv2D(pointwise_conv_filters, (1, 1), padding='same', strides=(1, 1))

    def __call__(self, inputs):
        if self.strides == (1, 1):
            x = inputs
        else:
            x = self.zero_padding_2d(inputs)

        x = self.depthwise_conv_layer(x)
        x = self.batch_norm(x)
        x = self.relu(x)
        x = self.conv2d(x)
        return self.relu(x)


def build_model(config, optimizer):
    '''
    Returns a compiled keras model with the MobileNet architecture from
    http://arxiv.org/abs/1704.04861. The input shape is fixed to the 
    96x96 grayscale pictures taked by the OV2640 camera. The number of 
    classes and the alpha parameter can be adjusted in the config. 
    '''
    model = tf.keras.Sequential()
    model.add(layers.InputLayer(input_shape=(96, 96, 1)))
    model.add(layers.Conv2D(filters=32 * config.alpha,
                            kernel_size=(3, 3),
                            strides=(2, 2),
                            use_bias=False, padding='same'))
    model.add(layers.BatchNormalization())
    model.add(layers.ReLU(max_value=6))
    model.add(DepthwiseConvBlock(64, config.alpha))
    model.add(DepthwiseConvBlock(128, config.alpha, strides=(2, 2)))
    model.add(DepthwiseConvBlock(128, config.alpha))
    model.add(DepthwiseConvBlock(256, config.alpha, strides=(2, 2)))
    model.add(DepthwiseConvBlock(256, config.alpha))
    model.add(DepthwiseConvBlock(512, config.alpha, strides=(2, 2)))
    model.add(DepthwiseConvBlock(512, config.alpha))
    model.add(DepthwiseConvBlock(512, config.alpha))
    model.add(DepthwiseConvBlock(512, config.alpha))
    model.add(DepthwiseConvBlock(512, config.alpha))
    model.add(DepthwiseConvBlock(512, config.alpha))
    model.add(DepthwiseConvBlock(1024, config.alpha, strides=(2, 2)))
    model.add(DepthwiseConvBlock(1024, config.alpha))
    model.add(layers.GlobalAveragePooling2D())
    model.add(layers.Dropout(config.dropout_rate))
    model.add(layers.Dense(config.classes))
    model.add(layers.Activation(activation='softmax'))

    loss = tf.keras.losses.CategoricalCrossentropy(
        from_logits=False,
        name='categorical_crossentropy')

    model.compile(optimizer=optimizer,
                  loss=loss,
                  metrics=[tf.keras.metrics.CategoricalAccuracy()])

    return model

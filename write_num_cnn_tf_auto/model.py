import keras
from keras import layers

def build_model(input_size, num_classes):

    inputs = keras.Input(shape=(150, input_size))

    # x = layers.LSTM(16, return_sequences=True, unroll= False)(inputs)
    # x = layers.LSTM(32, return_sequences=False, unroll= False)(x)

    x = layers.Conv1D(16, 3, activation='relu')(inputs)
    x = layers.MaxPooling1D(2)(x)
    x = layers.Dropout(0.1)(x)


    x = layers.Conv1D(32, 3, activation='relu')(x)
    x = layers.GlobalAveragePooling1D()(x)

    outputs = layers.Dense(num_classes,activation='softmax')(x)

    model = keras.Model(inputs, outputs)

    return model
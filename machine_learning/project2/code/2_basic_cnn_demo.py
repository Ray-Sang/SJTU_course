#!/usr/bin/env python
# coding: utf-8

import tensorflow as tf
tf.test.gpu_device_name()

tf.test.is_gpu_available()

from tensorflow.python.client import device_lib

# 列出所有的本地机器设备
local_device_protos = device_lib.list_local_devices()
# 打印
#     print(local_device_protos)

# 只打印GPU设备
[print(x) for x in local_device_protos if x.device_type == 'GPU']

# import moxing as mox
# import os
# mox.file.shift('os', 'mox')
# print(os.listdir('./'))

import tensorflow as tf
import numpy as np
from tensorflow import keras
from tensorflow.keras.preprocessing.image import ImageDataGenerator
from tensorflow.keras.optimizers import RMSprop
from tensorflow.keras.callbacks import TensorBoard

train_datagen=ImageDataGenerator(rescale=1./255)
train_generator=train_datagen.flow_from_directory(
    r"C:\Users\Ray\Desktop\dataset\train",
    target_size=(200,200),
    batch_size=32,
    class_mode='binary')

test_datagen=ImageDataGenerator(rescale=1./255)
validation_generator=test_datagen.flow_from_directory(
    r"C:\Users\Ray\Desktop\dataset\validation",
    target_size=(200,200),
    batch_size=32,
    class_mode='binary')

model=keras.models.Sequential([
    keras.layers.Conv2D(64,(3,3),activation='relu',input_shape=(200,200,3)),
    keras.layers.MaxPooling2D(2,2),
    keras.layers.Conv2D(32,(3,3),activation='relu'),
    keras.layers.MaxPooling2D(2,2),
    keras.layers.Conv2D(64,(3,3),activation='relu'),
    keras.layers.MaxPooling2D(2,2),
    keras.layers.Flatten(),
    keras.layers.Dense(512,activation='relu'),
    keras.layers.Dropout(0.5),
    keras.layers.Dense(1,activation='sigmoid')
    ])
model.summary();

tensorboard = TensorBoard(log_dir=".\logs") 

model.compile(loss='binary_crossentropy',
              #optimizer=RMSprop(lr=0.0001),
              optimizer='adam',
              metrics=['acc'])

model.fit(train_generator, epochs=5, callbacks=[tensorboard]) # steps_per_epoch=10, validation_steps=10)

loss, acc = model.evaluate(validation_generator,verbose=2)

model.save(".\model.h")

new_model = keras.models.load_model(r"C:\Users\Ray\Desktop\model.h")

loss, acc = new_model.evaluate(validation_generator,verbose=2)

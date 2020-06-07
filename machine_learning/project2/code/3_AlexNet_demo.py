#!/usr/bin/env python
# coding: utf-8

# # This file is for AlexNet demo

# ## See if gpu work
import tensorflow as tf
tf.test.gpu_device_name()

tf.test.is_gpu_available()

from tensorflow.python.client import device_lib

# 列出所有的本地机器设备
local_device_protos = device_lib.list_local_devices()
# 打印
# 只打印GPU设备
[print(x) for x in local_device_protos if x.device_type == 'GPU']


# ## Import tensorflow
import tensorflow as tf
import numpy as np
from tensorflow import keras
from tensorflow.keras.preprocessing.image import ImageDataGenerator
from tensorflow.keras.optimizers import RMSprop
from tensorflow.keras.callbacks import TensorBoard


# ## Generate the dataset
train_datagen = ImageDataGenerator(rescale=1./255)
train_generator = train_datagen.flow_from_directory(
    r".\train",
    target_size=(224,224),
    # batch_size=64,
    class_mode='binary')

test_datagen = ImageDataGenerator(rescale=1./255)
validation_generator = test_datagen.flow_from_directory(
    r".\validation",
    target_size=(224,224),
    # batch_size=64,
    class_mode='binary')

# ## Define alexnet model
model = keras.models.Sequential([
    keras.layers.Conv2D(64,(3,3),strides=(1,1),input_shape=(224,224,3),padding='same',activation='relu',kernel_initializer='uniform'),
    keras.layers.Conv2D(64,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'),  
    keras.layers.MaxPooling2D(pool_size=(2,2)), 
    keras.layers.Conv2D(128,(3,2),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'),  
    keras.layers.Conv2D(128,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'), 
    keras.layers.MaxPooling2D(pool_size=(2,2)), 
    keras.layers.Conv2D(256,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'), 
    keras.layers.Conv2D(256,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'), 
    keras.layers.Conv2D(256,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'),  
    keras.layers.MaxPooling2D(pool_size=(2,2)),
    keras.layers.Conv2D(512,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'),  
    keras.layers.Conv2D(512,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'),  
    keras.layers.Conv2D(512,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'), 
    keras.layers.MaxPooling2D(pool_size=(2,2)),  
    keras.layers.Conv2D(512,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'),  
    keras.layers.Conv2D(512,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'),  
    keras.layers.Conv2D(512,(3,3),strides=(1,1),padding='same',activation='relu',kernel_initializer='uniform'), 
    keras.layers.MaxPooling2D(pool_size=(2,2)),
    keras.layers.Flatten(),
    keras.layers.Dense(4096,activation='relu'),
    keras.layers.Dropout(0.5), 
    keras.layers.Dense(4096,activation='relu'),
    keras.layers.Dropout(0.5), 
    keras.layers.Dense(1,activation='softmax'),
    ])

model.summary();

# ## Define tensorboard
tensorboard = TensorBoard(log_dir=r".\logs_alexnet")

# ## Fit the model 
model.compile(loss='binary_crossentropy',
              optimizer=RMSprop(lr=0.001),
              metrics=['acc'])

model.fit(train_generator, epochs=5, callbacks=[tensorboard], steps_per_epoch=8) # , validation_steps=8)

# ## Open tensorboard
get_ipython().system(' tensorboard --logdir logs_alexnet')

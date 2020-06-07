'''
本程序实现卷积层输出的feature map的可视化
创建4*4子图，可显示指定某层输出的前16个feature map的可视化
'''
import numpy as np
import matplotlib.pyplot as plt
from tensorflow import keras
from keras.preprocessing import image

#载入图片，路径须自己指定
#化为数组并归一化
img1=image.load_img("path",
                   target_size=(200,200))
x1=image.img_to_array(img1)
x1=x1/255.0
x1=x1.reshape(1,200,200,3)

#这部分代码的基础是吴恩达团队的tensorflow2.0教学视频，并在此基础上进行一定的修改
model=keras.models.load_model('name of model')#加载模型
f,axarr=plt.subplots(4,4)#创建4*4子图
CON_NUM=0#表示某层的第几个feature map
LAYER_NUM=0#表示第几层，可取0到5，分别是三个卷积层和三个池化层，按顺序
layer_outputs=[layer.output for layer in model.layers]#layer.output表示得到model每一层的输出张量
activation_model=keras.models.Model(inputs=model.input,outputs=layer_outputs)
#activation_model是一个数组，里面包括model的input分别到6个层的output的6个模型，1*6
for x in range(0,4):
    f1=activation_model.predict(x1)[LAYER_NUM]
    axarr[x,0].imshow(f1[0,:,:,CON_NUM+1],cmap='inferno')
    axarr[x,0].grid(False)
    f2=activation_model.predict(x1)[LAYER_NUM]
    axarr[x,1].imshow(f2[0,:,:,CON_NUM+2],cmap='inferno')
    axarr[x,1].grid(False)
    f3=activation_model.predict(x1)[LAYER_NUM]
    axarr[x,2].imshow(f3[0,:,:,CON_NUM+3],cmap='inferno')
    axarr[x,2].grid(False)
    f4=activation_model.predict(x1)[LAYER_NUM]
    axarr[x,3].imshow(f4[0,:,:,CON_NUM+4],cmap='inferno')
    axarr[x,3].grid(False)
    CON_NUM=CON_NUM+4


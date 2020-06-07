'''
本程序实现CNN模型的CAM（类激活图）可视化，其中类激活图的描述见论文
可生成拥有2*2子图的图片，展示四张不同图片的CAM可视化
'''
import numpy as np
import matplotlib.pyplot as plt
from tensorflow import keras
from keras import backend as K
from keras.preprocessing import image
import cv2


#本函数用来实现指定图片通过指定模型的CAM可视化
#本函数代码基于https://www.jianshu.com/p/fb3add126da1的代码，并在此基础上进行修改
def output_heatmap(model, last_conv_layer, img):
    """Get the heatmap for image.

    Args:
           model: keras model.
           last_conv_layer: name of last conv layer in the model.
           img: processed input image.

    Returns:
           heatmap: heatmap.
    """
    # predict the image class
    preds = model.predict(img)
    # find the class index
    index = np.argmax(preds[0])
    # This is the entry in the prediction vector
    target_output = model.output[:, index]

    # get the last conv layer
    last_conv_layer = model.get_layer(last_conv_layer)

    # compute the gradient of the output feature map with this target class
    grads = K.gradients(target_output, last_conv_layer.output)[0]

    # mean the gradient over a specific feature map channel
    pooled_grads = K.mean(grads, axis=(0, 1, 2))

    # this function returns the output of last_conv_layer and grads 
    # given the input picture
    iterate = K.function([model.input], [pooled_grads, last_conv_layer.output[0]])
    pooled_grads_value, conv_layer_output_value = iterate([img])

    # We multiply each channel in the feature map array
    # by "how important this channel is" with regard to the target class
    for i in range(conv_layer_output_value.shape[-1]):
        conv_layer_output_value[:, :, i] *= pooled_grads_value[i]

    # The channel-wise mean of the resulting feature map
    # is our heatmap of class activation
    heatmap = np.mean(conv_layer_output_value, axis=-1)
    heatmap = np.maximum(heatmap, 0)
    heatmap /= np.max(heatmap)

    return heatmap

model=keras.models.load_model("name of model")#加载模型
model.summary()#用summary函数显示模型的详细信息，得到最后一层的名称

#加载img1到image4四张图片，并进行归一化
#若不归一化，会导致RELU函数计算得到的值和1或0差别过小，导致函数报错
#图片路径path须自行指定
img1=image.load_img("path",
                   target_size=(200,200))
x1=image.img_to_array(img1)
x1=x1/255.0
x1=x1.reshape(1,200,200,3)

img2=image.load_img("path",
                   target_size=(200,200))
x2=image.img_to_array(img2)
x2=x2/255.0
x2=x2.reshape(1,200,200,3)

img3=image.load_img("path",
                   target_size=(200,200))
x3=image.img_to_array(img3)
x3=x3/255.0
x3=x3.reshape(1,200,200,3)

img4=image.load_img("path",
                   target_size=(200,200))
x4=image.img_to_array(img4)
x4=x4/255.0
x4=x4.reshape(1,200,200,3)

f,axarr=plt.subplots(2,2)#创建2*2子图

#调用output_heatmap函数，得到类激活图
heatmap1=output_heatmap(model,'name',x1)
heatmap2=output_heatmap(model,'name',x2)
heatmap3=output_heatmap(model,'name',x3)
heatmap4=output_heatmap(model,'name',x4)

#将生成的子图填上热图形式的类激活图
axarr[0,0].imshow(heatmap1,cmap='inferno')
axarr[0,1].imshow(heatmap2,cmap='inferno')
axarr[1,0].imshow(heatmap3,cmap='inferno')
axarr[1,1].imshow(heatmap4,cmap='inferno')
'''
#这部分代码可选
#作用是将生成的类激活图叠加到原图上，使得更清晰直观
img1 = cv2.imread('path')#加载原图
heatmap = cv2.resize(heatmap, (img1.shape[1],img1.shape[0]))#将原图调整到指定大小，由对应的类激活图决定
heatmap = np.uint8(255 * heatmap)#将类激活图从0~1映射到0~255，方便作图
heatmap = cv2.applyColorMap(heatmap, cv2.COLORMAP_JET)#类激活图添加颜色
superimposed_img = heatmap * 0.5 + img1 #将类激活图与原图组合在一起
cv2.imwrite('path', superimposed_img)#不能用中文路径，这个地方要特别注意
'''

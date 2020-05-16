# -*- coding: utf-8 -*-

import numpy as np
from sklearn.svm import SVC
from sklearn.model_selection import train_test_split
from sklearn.multiclass import OneVsRestClassifier
import matplotlib.pyplot as plt

train_data = np.load('D:/sjtu/sjtu_files/机器学习/作业/first/data_copy/train_data.npy', encoding='latin1')
train_label = np.load('D:/sjtu/sjtu_files/机器学习/作业/first/data_copy/train_label.npy', encoding='latin1')
test_data = np.load('D:/sjtu/sjtu_files/机器学习/作业/first/data_copy/test_data.npy', encoding='latin1')
test_label = np.load('D:/sjtu/sjtu_files/机器学习/作业/first/data_copy/test_label.npy', encoding='latin1')

# 打乱训练数据
X_train = np.array(train_data)
y_train = np.array(train_label)
np.random.seed(116)
np.random.shuffle(X_train)
np.random.seed(116)
np.random.shuffle(y_train)
#print(X_train, y_train)

train_num = 1200
# train the model
model_ovr = SVC(C=6, kernel='rbf', gamma=0.0001, degree=3, decision_function_shape='ovr')
clf_ovr = model_ovr.fit(X_train[0:train_num,:], y_train[0:train_num])

print(clf_ovr.score(X_train, y_train))
print(clf_ovr.score(test_data, test_label))

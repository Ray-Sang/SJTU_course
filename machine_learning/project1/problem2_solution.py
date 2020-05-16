import numpy as np
from sklearn.svm import SVC

#注意：要根据文件实际位置修改
train_data = np.load('D:/sjtu/sjtu_files/机器学习/作业/first/data_copy/train_data.npy', encoding='latin1')
train_label = np.load('D:/sjtu/sjtu_files/机器学习/作业/first/data_copy/train_label.npy', encoding='latin1')
test_data = np.load('D:/sjtu/sjtu_files/机器学习/作业/first/data_copy/test_data.npy', encoding='latin1')
test_label = np.load('D:/sjtu/sjtu_files/机器学习/作业/first/data_copy/test_label.npy', encoding='latin1')

## 数据预处理，每组label有3000个训练样本
# 打乱训练数据
x_train = np.array(train_data)
y_train = np.array(train_label)
np.random.seed(116)
np.random.shuffle(x_train)
np.random.seed(116)
np.random.shuffle(y_train)
x_test = np.array(test_data)
y_test = np.array(test_label)
np.random.seed(116)
np.random.shuffle(x_test)
np.random.seed(116)
np.random.shuffle(y_test)

# set1，set2，set3分别是train_label为1，0，-1的train_data元素组成的集合
set1 = np.zeros((1,310))
set2 = np.zeros((1,310))
set3 = np.zeros((1,310))

#先取1200个数据作为训练集
number = 1200
num_sub = round(number/3) - 20
for i in range(number): 
    if y_train[i] == 1:
        set1 = np.vstack((set1, x_train[i,:])) # vstack函数是个纵向拼接函数，a=np.vstack((c,b))代表a由c和b纵向拼接而成
    if y_train[i] == 0:
        set2 = np.vstack((set2, x_train[i,:]))
    if y_train[i] == -1:
        set3 = np.vstack((set3, x_train[i,:]))  
        
#把第一行的[0,0,0,...,0]删去
set1 = np.delete(set1, 0, axis=0)
set2 = np.delete(set2, 0, axis=0)
set3 = np.delete(set3, 0, axis=0) 

#取前num_sub个数据
set1 = np.array(set1[:num_sub, :])
set2 = np.array(set2[:num_sub, :])
set3 = np.array(set3[:num_sub, :])
sets = [set1, set2, set3]

symbol = np.zeros((3,3,10000))
# 只循环三次
rho = 30 # 设置每个子集个数
for i in range(2):
    for j in range(3):
        # i:0,1  j:1,2,[i,j]=[0,1]/[0,2]/[1,2]
        if j <= i:
            continue
        
        positive_set = sets[i]
        shape_positive = positive_set.shape[0]
        num_of_positive_subsets = round(shape_positive/rho)
        positive_subsets = np.array_split(positive_set, num_of_positive_subsets, axis=0)
        
        negative_set = sets[j]
        shape_negative = negative_set.shape[0]
        num_of_negative_subsets = round(shape_negative/rho)
        nagetive_subsets = np.array_split(negative_set, num_of_negative_subsets, axis=0)
        
        predict_vals = np.zeros((num_of_positive_subsets, num_of_negative_subsets, 10000))
        for p in range(num_of_positive_subsets):
            for n in range(num_of_negative_subsets):
                shape1 = positive_subsets[p].shape[0]
                shape2 = nagetive_subsets[n].shape[0]
                
                # 设置每一个子向量机的训练数据
                x = np.vstack((positive_subsets[p], nagetive_subsets[n]))
                y_p = np.ones((shape1, 1))
                y_n = -np.ones((shape2, 1))
                y = np.vstack((y_p, y_n))
                y = y.ravel()
                
                # 学习子向量机并预测
                clf = SVC(kernel='linear', degree=3, gamma='auto', decision_function_shape='ovo')
                clf.fit(x, y)
                predict_val = clf.predict(x_test[:10000, :])
                for m in range(10000):
                    predict_vals[p][n][m] = predict_val[m]
                    
        for m in range(10000):
            # 取每行最小
            vector = predict_vals[:,:,m].min(axis=1)
            # 每列取最大
            max_val = vector.max()
            
            if max_val == 1:
                symbol[i][j][m] = i + 1
            else:
                symbol[i][j][m] = j + 1
        
        
# 投票        
voter = np.zeros((10000,3))
for m in range(10000):
    for i in range(2):
        for j in range(3):
             # i:0,1  j:1,2,[i,j]=[0,1]/[0,2]/[1,2]
            if j <= i:
                continue
            
            if symbol[i][j][m] == 1:
                voter[m][0] = voter[m][0] + 1
            if symbol[i][j][m] == 2:
                voter[m][1] = voter[m][1] + 1    
            if symbol[i][j][m] == 3:
                voter[m][2] = voter[m][2] + 1    
     
predict_label = voter.argmax(axis=1) + 1
#print(predict_label)
#print(y_test[:10000,])

#正确率计算
right = 0     
for m in range(10000):
    if (predict_label[m]==1 and y_test[m]==1)or(predict_label[m]==2 and y_test[m]==0)or(predict_label[m]==3 and y_test[m]==-1):
        right = right+1
ratio = right/10000

print("预测准确率：")
print(ratio)   
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

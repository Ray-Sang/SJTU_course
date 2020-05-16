% 此文件是案例 1 的代码文件

tic
clear;

iteration_flag = 1;
cnt = 0; % 记录迭代次数
y_cost = []; % 记录每次迭代之后的测定成本

% 创建初始种群,大小为100*90
group = get_initial_group();

% 读入训练数据
training_data = dlmread("dataform_train-0229.csv"); % 大小为1000*90，有500组T/V关系，首行是温度，次行是电压

cost = get_cost(group,training_data); % 对数据进行三次样条插值，分别对100个样本计算标定成本，返回一个1*100的向量，记录每个个体的标定成本

% 明确，我们要根据电压作为自变量拟合出温度与电压的关系，电压是自变量！
while iteration_flag
    cnt = cnt + 1; % 记录迭代次数
    
    % 计算适应度
    adaption = get_adaption(cost);
    
    % 淘汰
    group = eliminate(group,adaption);
    
    % 交叉互换，注意会引起数据点少于两个的情况
    group = exchange(group);
    
    % 变异，注意会引起数据点少于两个的情况
    group = mutate(group);
    
    % 检查配对、变异是否引起数据点少于两个的情况
    group = check_ones(group);
    
    cost = get_cost(group,training_data); % 对数据进行三次样条插值，分别对100个样本计算标定成本，返回一个1*100的向量，记录每个个体的标定成本
    
    y_cost = [y_cost min(cost)]; % 记录每次迭代后的最小平均测定成本
    
    % 判断是否退出循环
    iteration_flag = is_to_jump_out(y_cost);
end

% 输出最小的平均标定成本和标定点
point = print_result(group,cost); % 返回一个1*90的向量，记录着为1的数据点
fprintf("Iterated %d times.\n",size(y_cost,2));
x_iterate = 1:1:cnt;

% 画迭代曲线
plot(x_iterate,y_cost,'x--');
xlabel('Iteration times');
ylabel('Calibration cost');
title('The curve of iteration');

% 测试测试集数据
test(point);

toc

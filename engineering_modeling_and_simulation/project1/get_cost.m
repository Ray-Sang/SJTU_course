function average_cost = get_cost(group,training_data)
% 对数据进行拟合、三次样条插值，分别对500个数据计算标定成本，返回一个1*100的向量，记录每个个体的标定成本

Q = 50; % 设置单次测定成本
average_cost = zeros(1,size(group,1));

% 对200种标定方案的每一个样本都进行计算标定成本
for k = 1:size(group,1)
    % 对500组中，每一组t v数据进行选点，拟合，插值，并计算成本，最后求得改标定方法的平均成本
    cost_sum = 0; % 此种标定情况下，成本的总和
    count = 0; % 累加测定次数
    y = []; % 保存选中的测定点的温度
    for m = 1:size(group,2)
        if group(k,m) == 1
            count = count + 1;
            y = [y m-21];
        end
    end
    
    for n = 1:(size(training_data,1)/2)
        % 将样本选择的点的电压值（横坐标）保存在向量 x 中
        x = [];
        cost = 0;
        for m = 1:size(group,2)
            if group(k,m) == 1
                tmp = training_data(2*n,m);
                x = [x tmp];
            end
        end
    
        % 对选中的数据点进行三次样条插值拟合
        yy = spline(x,y,training_data(2*n,:)); % 储存拟合后的温度值 
        del_temperature = abs(yy - training_data(2*n-1,:)); % del_temperature保存的是拟合后的温度差值
    
        % 计算单次、对一组数据的标定成本
        cost = count * Q;
        for m = 1:size(del_temperature,2)
            if del_temperature(1,m) <= 0.5
                continue;
            elseif del_temperature(1,m) > 0.5 && del_temperature(1,m) <=1
                cost = cost + 1;
            elseif del_temperature(1,m) > 1 && del_temperature(1,m) <=1.5
                cost = cost + 5;
            elseif del_temperature(1,m) > 1.5 && del_temperature(1,m) <=2.0
                cost = cost + 10;
            else
                cost = cost + 10000;
            end
        end
        
        % 对每组成本进行累加
        cost_sum = cost_sum + cost;
    end
    
    % 计算对500组数据标定的平均成本，并存储到average_cost对应的位置
    average_cost(1,k) = cost_sum/(size(training_data,1)*0.5);
end
end

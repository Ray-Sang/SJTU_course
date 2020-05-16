function min_result = print_result(group,cost)
% 输出执行结果，返回1*100向量记录要选择的点

min_cost = min(cost);
[m,n] = find(cost == min_cost);
min_result2 = group(n,:); % 如果有多个相同的值，那么find返回也是一个矩阵！每行内容会相同
min_result = min_result2(1,:);
temperature_point = [];

for k = 1:size(min_result,2)
    if min_result(1,k) == 1
        temperature_point = [temperature_point k-21];
    end
end

fprintf("The minimun cost is: %f4.\n",min_cost);
disp("The chosen temperature point is :");disp(temperature_point);

end

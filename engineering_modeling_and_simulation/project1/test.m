function test(point)
% 使用选定的标定方案，计算测试集中的平均标定成本

test_data = dlmread("dataform_testA-0229.csv");
cost_test = get_cost(point,test_data);
fprintf("\nThe test data's average cost is %f4.",cost_test);

end

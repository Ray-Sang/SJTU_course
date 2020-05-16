function flag = is_to_jump_out(y_cost)
% 判断是否要退出迭代（是否收敛）
% 若y_cost最后四个数据的标准差小于5，就认为收敛，可以退出

cost_size = size(y_cost,2);
if cost_size > 4
    a = [y_cost(1,cost_size-3) y_cost(1,cost_size-2) y_cost(1,cost_size-1) y_cost(1,cost_size)];
    
    if std(a) < 5
        flag = 0;
    else
        flag = 1;
    end
else
    flag = 1;
end

end

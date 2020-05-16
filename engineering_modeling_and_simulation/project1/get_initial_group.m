function group = get_initial_group()
% 获取初始种群

group = zeros(100,90);
possibility_chosen = 0.40; % 选择为1的可能性可以任意，没有很大差别

% 对每一个元素进行遍历，储存1/0
for k = 1:size(group,1)
    cnt = 0;
    for m = 1:size(group,2)
        if rand() <= possibility_chosen
            group(k,m) = 1;
            cnt = cnt + 1;
        end
    end
    
    % 避免全部是0或者只有一个1的情况
    if cnt == 0 || cnt == 1
        for m = 1:size(group,2)
            if group(k,m) == 0
               group(k,m) = 1;
               cnt = cnt + 1;
            end
            if cnt == 2
               break;
            end
        end
    end
end
end

function group = check_ones(group)
% 检查是否不满足三次样条插值的情况

for k = 1:size(group,1)
    cnt = 0;
    for m = 1:size(group,2)
        if group(k,m) == 1
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

function group = exchange(group)
% 交叉互换的代码实现

group_size = size(group,1); % 取出group的行数

for k = 1:1/2 * group_size
    % 选择交换的点，设置成在20~70之间的数值
    position = ceil(rand()*70) - ceil(rand()*20);
    
    % 基因交叉配对（片段交换）
    for j = 1:position
        medium = group(k,j); % 用作复制的中间环节，a与b交换的临时介质
        group(k,j) = group(group_size - k + 1,j);
        group(group_size - k + 1,j) = medium;
    end
end

end

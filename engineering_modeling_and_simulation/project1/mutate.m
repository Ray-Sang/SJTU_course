function group = mutate(group)
% 变异的实现

% 这样设置的变异概率，对于1变0是0.08,0变1是0.02
possibility = 0.2;
possibility_0 = 0.1; % 0变1的概率
possibility_1 = 0.9; % 1变0的概率

for k = 1:size(group,1)
   for m = 1:size(group,2)
       if rand() < possibility
           if group(k,m)
               if rand() < possibility_1
                   group(k,m) = 0;
               end
           else
               if rand() < possibility_0
                    group(k,m) = 1;
               end
           end
       end
   end
end

end

function adaption = get_adaption(cost)
% 根据标定成本计算适应度
% 返回一个1*100的向量，储存每个标定方案的适应度大小

adaption = zeros(1,size(cost,2));
% solution:如果cost大于800，那么适应度大小就设置成100；如果小于800，适应度大小就是2^(800-cost)
for k = 1:size(cost,2)
   if cost(1,k) > 800
       adaption(1,k) = 100;
   else
       adaption(1,k) = 2^(800-cost(1,k));
   end
end

adaption_sum = sum(adaption);
adaption = adaption ./ adaption_sum;

end

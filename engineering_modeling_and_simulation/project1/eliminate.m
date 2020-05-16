function new_group = eliminate(group,adaption)

new_group = zeros(size(group,1),size(group,2));
round = zeros(1,size(group,1));
flag = 1;

for k = 1:size(group,1)
    for m = 1:k
       round(1,k) = round(1,k) + adaption(1,m);  
    end
end

while flag < size(group,1) + 1
   tmp = rand();
   for k = 1:size(round,2)
      if tmp <= round(1,k)
          new_group(flag,:) = group(k,:);
          flag = flag + 1;
          break;
      end
   end
end

end

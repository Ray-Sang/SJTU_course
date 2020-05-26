k = 5;
w = 25000;
lambdaA = 1/2.72e4;
lambdaB = 1/3.32e5;

% Calculate the possibility of A&B
A_state_0 = exp(-w * lambdaA);
A_state_1 = 0.3 * (1 - A_state_0);
A_state_2 = 0.3 * (1 - A_state_0);
A_state_3 = 0.4 * (1 - A_state_0);
B_state_0 = exp(-w * lambdaB);
B_state_1 = 0.33 * (1 - B_state_0);
B_state_2 = 0.67 * (1 - B_state_0);

% pf, so, dm, mo, dn, fb
p_node_state = [A_state_0*B_state_0 A_state_0*B_state_2+A_state_1*B_state_0+A_state_1*B_state_2 A_state_2*B_state_0 A_state_0*B_state_1+A_state_2*B_state_1 A_state_2*B_state_2+A_state_3*(B_state_0+B_state_1+B_state_2) A_state_1*B_state_1];

availability = [];

for n =  5:20
    % 穷举结点状态分布组合
    node_state_num = []; 
    for n1 = 0:n
        for n2 = 0:n-n1
            for n3 = 0:n-n1-n2
                for n4 = 0:n-n1-n2-n3
                    for n5 = 0:n-n1-n2-n3-n4
                        for n6 = 0:n-n1-n2-n3-n4-n5
                            if n1+n2+n3+n4+n5+n6 == n
                                node_state_num = [node_state_num; n1 n2 n3 n4 n5 n6];
                            end
                        end
                    end
                end
            end
        end
    end
    
    node_size_x = size(node_state_num, 1);
    final_p = 0; 
    for i = 1:node_size_x
        node_num = node_state_num(i,:);
        pf = node_num(1);
        so = node_num(2);
        dm = node_num(3);
        mo = node_num(4);
        dn = node_num(5);
        fb = node_num(6);
        if fb==0 && ((mo==1 && pf+so>=k-1) ||((mo==0 && pf>=1 && (pf+so)>=k) || (mo==0 && pf==0 && dm>=1 && so>=k-1)))
            p = p_node_state .^ node_num;
            possibility = nchoosek(n,pf)*nchoosek(n-pf,so)*nchoosek(n-pf-so,dm)*nchoosek(n-pf-so-dm,mo)*nchoosek(n-pf-so-dm-mo,dn)*nchoosek(n--pf-so-dm-mo-dn,fb)*p(1)*p(2)*p(3)*p(4)*p(5)*p(6);
            final_p = final_p + possibility;
        end
        if (fb+mo)==0 && (pf>=1 && pf+so==k-1 && dm>=1) && (rand()<=dm / (dm + pf))
            p = p_node_state .^ node_num;
            possibility = (dm/(dm+pf))*nchoosek(n,pf)*nchoosek(n-pf,so)*nchoosek(n-pf-so,dm)*nchoosek(n-pf-so-dm,mo)*nchoosek(n-pf-so-dm-mo,dn)*nchoosek(n--pf-so-dm-mo-dn,fb)*p(1)*p(2)*p(3)*p(4)*p(5)*p(6);
            final_p = final_p + possibility;
        end 
    end
    availability=[availability final_p];
end
        
for n = 1:16
    fprintf("结点总数为%d时，系统的可用性为%f.4.\n", n+4, availability(n));
end
        
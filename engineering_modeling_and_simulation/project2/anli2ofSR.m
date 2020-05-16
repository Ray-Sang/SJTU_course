% 切换器 A 的使用寿命Ta指数分布，1/lambdaA = 2.72e4
% 条件概率 Pa1=Pa2=0.3, Pa3=0.4
% 切换器 B 的使用寿命Tb指数分布，1/lambdaA = 3.32e5
% 条件概率 Pb1=0.33, Pb2=0.67
% 最少的工作结点数为k=5
% 元件一旦发生故障，故障类型确定，不变；故障均不可修复
% A 在 t 时刻正常工作的概率：exp(-lambda * t), 故障的总概率：1 - exp(-lambda * t)
% B 同 A
% 模拟s套系统的运行状况，s>=1e5

tic

k = 5;
w = 25000;
total_number_of_system = 1e5;
Rw = zeros(1,20); % 存储可靠性
Et = zeros(1,20); % 存储平均工作寿命
lambdaA = 1/2.72e4;
lambdaB = 1/3.32e5;
n = 5;

for num_of_node = 5:20 % 结点总数为5~20
    work_time = zeros(1,total_number_of_system);
    for num_of_sys = 1:total_number_of_system % 使用蒙特卡洛法，模拟10000套系统的运行状态
        Ta = exprnd(1/lambdaA, 1, num_of_node);
        Tb = exprnd(1/lambdaB, 1, num_of_node);
        
        % 元件 A/B 的状态
        % 判断各个结点中，元件 A 的故障种类
        A_error = ones(1,num_of_node);
        for i = 1:num_of_node
            random_num = rand();
            if random_num <=0.3
                continue;
            elseif random_num <= 0.6
                A_error(i) = 2;
            else
                A_error(i) = 3;
            end
        end
        % 判断各个结点中，元件 B 的故障种类
        B_error = ones(1,num_of_node);
        for i = 1:num_of_node
            random_num = rand();
            if random_num <=0.33
                continue;
            else
                B_error(i) = 2;
            end
        end
        
        node_state = zeros(1, num_of_node); % store the states of each node
        A_state = zeros(1, num_of_node); % store the states of A 
        B_state = zeros(1, num_of_node); % store the state of B 
        % The state of node is determined by the state of A/B, so we only
        % consider the time when A/B changes its own state
        times = [Ta Tb];
        % sort() 改变原顺序，按照数字从大到小排列原list，并将排列后的每个元素的原索引保存在index中，从1开始indexing
        [times, index] = sort(times);
        
        for i = 1:2*num_of_node % 每一个时间结点都需要判断
            % 先把 A,B 元件的“此时刻的”状态搬运到 A_state & B_state 中
            label_of_node = index(i); % 默认是 A 结点的结点标号；是 B ，则需映射
            if label_of_node <= num_of_node
                % 说明这个时间点是元件 A 的状态发生改变
                A_state(label_of_node) = A_error(label_of_node);
            else
                % 此时间点元件 B 的状态发生改变
                label_of_node = label_of_node - num_of_node; % 把 num_of_node+1 ~ 2*num_of_node 映射到 1 ~ num_of_node
                B_state(label_of_node) = B_error(label_of_node);
            end
            
            % Determine the state of nodes from A&B of each node
            if A_state(label_of_node) == 0
                if B_state(label_of_node) == 0
                    node_state(label_of_node) = 0;
                elseif B_state(label_of_node) == 1
                    node_state(label_of_node) = 3;
                else
                    node_state(label_of_node) = 1;
                end
            elseif A_state(label_of_node) == 1
                if B_state(label_of_node) == 0
                    node_state(label_of_node) = 1;
                elseif B_state(label_of_node) == 1
                    node_state(label_of_node) = 5;
                else
                    node_state(label_of_node) = 1;
                end
            elseif A_state(label_of_node) ==2
                if B_state(label_of_node) == 0
                    node_state(label_of_node) = 2;
                elseif B_state(label_of_node) == 1
                    node_state(label_of_node) = 3;
                else
                    node_state(label_of_node) = 4;
                end
            else
                if B_state(label_of_node) == 0
                    node_state(label_of_node) = 4;
                elseif B_state(label_of_node) == 1
                    node_state(label_of_node) = 4;
                else
                    node_state(label_of_node) = 4;
                end
            end
            
            % Calculate the amount of different state of nodes
            pf = 0; % perfectly functioning
            so = 0; % slave only
            dm = 0; % disable/master
            mo = 0; % master only
            dn = 0; % disable node
            fb = 0; % failed bus
            for m = 1:num_of_node
                if node_state(m) == 0
                    pf = pf + 1;
                elseif node_state(m) == 1
                    so = so + 1;
                elseif node_state(m) == 2
                    dm = dm + 1;
                elseif node_state(m) == 3
                    mo = mo + 1;
                elseif node_state(m) == 4
                    dn = dn + 1;
                else
                    fb = fb + 1;
                end
            end
            
            % Determine the state of the system from the state of the nodes:
            sys_state = 0;
            if fb>=1 || mo>=2 || (pf+mo+dm)==0 || (pf+so+((mo+dm)>0))<k
                sys_state = 1;
            elseif fb==0 && ((mo==1 && pf+so>=k-1) ||((mo==0 && pf>=1 && (pf+so)>=k) || (mo==0 && pf==0 && dm>=1 && so>=k-1)))
                sys_state = 2;
            elseif (fb+mo)==0 && (pf>=1 && pf+so==k-1 && dm>=1)
                possibility = dm / (dm + pf);
                if rand() <= possibility
                    sys_state = 3;
                else
                    sys_state = 4;
                end
            end
            
            % Whether to jump out and record time
            total_time = 0;
            if sys_state == 1 || sys_state == 4
                total_time = times(i);
                break;
            end
        end
        
        % Revise the bug
        if total_time == 0 || total_time > 90000
            total_time = 90000;
        end
        
        work_time(num_of_sys) = total_time;
    end
    
    % calculate the reliability
    reliable_sys = 0;
    for i = 1:total_number_of_system
        if work_time(i) >= w
            reliable_sys = reliable_sys + 1;
        end
    end
    % store the reliability and the average working time of the system
    Rw(num_of_node) = reliable_sys/total_number_of_system;
    Et(num_of_node) = sum(work_time)/total_number_of_system;
end

% print
max_Rw = max(Rw);
max_num_of_node_Rw = find(Rw==max_Rw, 1);
max_Et = max(Et);
max_num_of_node_Et = find(Et==max_Et, 1);

fprintf('最大可靠性为%7.4f.\n', max_Rw)
fprintf('此时对应节点数为%d.\n', max_num_of_node_Rw)
fprintf('最大平均工作寿命为%7.4f.\n', max_Et)
fprintf('此时对应节点数为%d.\n', max_num_of_node_Et)
toc

% x = 5:1:20;
% y = Et(5:20);
% plot(x,y,':.g');
% xlabel('结点数量');
% ylabel('系统工作寿命');

% y = Et(5:20);
% plot(x,y,':.g');


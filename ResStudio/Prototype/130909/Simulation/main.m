%% Parameters
nN= 12; % number of nodes
nT= 12; % number of tasks
nD = 3; % number of data in each task
nC = 4; % number of contexts
%% Column Indices
CONTX_COL=1;TASK_READER_COL=3;TASK_HOST_COL=4;TASK_SEQ_COL=2;
WRITE_DATA_COL = nD+4;

%%Decision variables
% task_host_policy :: THP_XXX will read as tasks will be hosted by XXX
THP_DATA_OWNER = 1;
% task_read_policy : TRP_YYY will read as tasks will be read by YYY
TRP_TASK_OWNER = 1;
TRP_BY_CONTEXT = 2;
%% ----------
task_host_policy = THP_DATA_OWNER;
task_read_policy = TRP_TASK_OWNER;
% task_read_policy = TRP_BY_CONTEXT;
for task_read_policy =1:2
    DH=[];
    for iC=1:nC
        DH = [DH;randi([iC-1 iC].*[nN/nC nN/nC]+[1 0] ,nT/nC,nD)];
    end
    prog=zeros(nT,4+nD);
    prog(:,5:end)=DH(:,:);
    prog(:,TASK_SEQ_COL)=1:nT;
    prog(:,CONTX_COL) = floor((0:nT-1) * nC / nT)+1;
    if task_host_policy == THP_DATA_OWNER 
        prog(:,TASK_HOST_COL) = prog(:,WRITE_DATA_COL);
    end
    if task_read_policy == TRP_TASK_OWNER
        prog(:,TASK_READER_COL)=prog(:,TASK_HOST_COL);
    end 
    if task_read_policy == TRP_BY_CONTEXT
        prog(:,TASK_READER_COL)=mod(prog(:,CONTX_COL),nN);
    end
    if size(prog,1) <= 20
        fprintf('     C    T#   TRdr    Th    D1:Dn ... \n')
        disp(prog) 
    end
    %% Communication costs

    diff_hosts1 = prog(:,TASK_HOST_COL) - prog(:,TASK_READER_COL);
    task_sent   = length ( find ( diff_hosts1 ~= 0 ) ) ;

    diff_hosts2 = prog(:,TASK_READER_COL) - prog(:,WRITE_DATA_COL);
    propagation_sent = length ( find ( diff_hosts2 ~= 0 ) );
    
    fprintf('Prop.# %4d,TaskSent# %4d,TaskRead %4d \n',propagation_sent,task_sent,nT)
    
    
end


% geometry

dim=2;
Nx=10; Ny=10;
% uniform grid from (0,0) to (1,1) with Nx*Ny points
[xx,yy] = meshgrid( linspace(0,1,Nx), linspace(0,1,Ny) );

% boundary point sets
pset_op(1:4)='0';
pset{1}=[xx(1,1:end-1)' yy(1,1:end-1)'];
pset{2}=[xx(2:end,end) yy(1:end-1,end)];

pset{3}=[xx(end,2:end)' yy(end,1:end-1)'];

pset{4}=[xx(2:end,1) yy(2:end,1)];


nsx = floor(Nx/2); % Midpoint
nsy = floor(Ny/2); % 

% interior point sets
pset_op(5:8)='L';
x=xx(2:nsy,2:nsx); y=yy(2:nsy,2:nsx);
pset{5}=[x(:) y(:)];
x=xx(2:nsy,nsx+1:end-1); y=yy(2:nsy,nsx+1:end-1);
pset{6}=[x(:) y(:)];
x=xx(nsy+1:end-1,2:nsx); y=yy(nsy+1:end-1,2:nsx);
pset{7}=[x(:) y(:)];
x=xx(nsy+1:end-1,nsx+1:end-1); y=yy(nsy+1:end-1,nsx+1:end-1);
pset{8}=[x(:) y(:)];

% write geometry to file
fid = fopen('boxgeom.dat','wt');
npts = 0; i = 0;
for i=1:length(pset); npts = npts + length(pset{i}); end

fprintf(fid, '%d %d %d\n', npts, dim, length(pset));
for i=1:length(pset)
    fprintf(fid, '%d %d\n', i-1, length(pset{i}));
    fprintf(fid, '%f %f\n', pset{i}');
    nfid = fopen(['input/boxgeometry' num2str(i) '.dat'],'wt');
    fprintf(nfid, '%d %d %d\n', length(pset{i}), dim, 1);
    fprintf(nfid, '%d %d\n', i-1, length(pset{i}));
    fprintf(nfid, '%f %f\n', pset{i}');
    fclose(nfid);

    pset{i}'
end
fclose(fid);

%stop
clear x2

% reread
fid = fopen('boxgeom.dat','rt');
h=fscanf(fid, '%d %d %d\n', [1,3]);
h3=h(3);
for i=1:h3
    sh = fscanf(fid, '%d %d\n', [1,2]);
    pset{i} = fscanf(fid, '%f %f\n', [2, sh(2)])';
end


% manufactured solution u(x,y)=x^2
sol = @(x) x(:,1).^2;
lsol = @(x) 2*ones(size(x(:,1)));

disp('rhs'); tic
% build right-hand-side
f=[ ];
for i=1:length(pset);
    if pset_op(i) == '0'
        f=[f; sol(pset{i})];
    else
        f=[f; lsol(pset{i})];
    end
end
toc

% rbf
epsilon=2;
gauss=@(r) exp(-(epsilon*r).^2);
lgauss=@(r) (-2*dim*epsilon^2 + 4*epsilon^4*(r.^2)).*exp(-(epsilon*r).^2);

disp('A');
tic
A=[];
rt=zeros(0, npts);
for i=1:length(pset)
    % distance matrix from this point set to all points
    r = [];
    for j=1:length(pset)
        [cx,rx]=meshgrid(pset{j}(:,1), pset{i}(:,1));
        [cy,ry]=meshgrid(pset{j}(:,2), pset{i}(:,2));
        r1 = sqrt((cx-rx).^2+(cy-ry).^2);
        r = [r r1];
    end
    rt = [rt; r];

    if pset_op(i) == '0'
        A = [A; gauss(r)];
    else
        A = [A; lgauss(r)];
    end
end
toc
%surf(xx,yy,A)
%    A=load('output.dat');

disp('B');
tic
% Evaluation matrix
B = gauss(rt);
toc

disp('solve');
tic
% Solve the system to find the weights
lambda = A\f;
toc

disp('evaluate');
tic
% Evaluate the solution
rbfsol = B*lambda;
toc

disp('true sol');
tic
% Calculate true solution
for i=1:length(pset)
    truesol{i} = sol(pset{i});
end
toc

disp('plot');
% plot solution
clf
hold on
idx = 1;
for i=1:length(pset)
    x = pset{i}(:,1);
    y = pset{i}(:,2);
    z = rbfsol(idx:idx+length(pset{i})-1);
    plot3(x, y, truesol{i}, 'k.');
    plot3(x, y, z, 'bo')
    idx = idx + length(pset{i});

    for j=1:length(x)
        plot3([x(j); x(j)], [y(j); y(j)], ...
              [z(j); truesol{i}(j)],'r');
    end
end

figure

% plot error
idx = 1;
for i=1:length(pset)
    x = pset{i}(:,1);
    y = pset{i}(:,2);
    z = rbfsol(idx:idx+length(pset{i})-1);
    plot3(x, y, truesol{i}-z, 'k.');
    hold on;
    idx = idx + length(pset{i});
end

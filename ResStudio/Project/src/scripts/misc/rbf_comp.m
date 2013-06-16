% geometry

dim=2;
Nx=10; Ny=10;
% uniform grid from (0,0) to (1,1) with Nx*Ny points
[xx,yy] = meshgrid( linspace(0,1,Nx), linspace(0,1,Ny) );

% boundary point sets
pset_op(1:4)='0';
pset{1}=[xx(1,1:end-1)' yy(1,1:end-1)']
pset{2}=[xx(2:end,end) yy(1:end-1,end)]

pset{3}=[xx(end,2:end)' yy(end,1:end-1)']

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

npts = 0;
for i=1:length(pset); npts = npts + length(pset{i}); end

tt = [];
for k = 1:2
    cnt = 9;
    for n = k:2:8
        if n >= 5 
            cnt = 16;
        end 
        ['./output/boxgeom' num2str(n) '.dat']
        fid = fopen(['./output/boxgeom' num2str(n) '.dat'],'rt');
        t= fscanf(fid,'%f',[100,cnt])';
        fclose(fid)
        tt = [tt;t];
    end 
end
i = 0;
% rbf
epsilon=2;
gauss=@(r) exp(-(epsilon*r).^2);
lgauss=@(r) (-2*dim*epsilon^2 + 4*epsilon^4*(r.^2)).*exp(-(epsilon*r).^2);

disp('A');
tic
A=[];
rt=zeros(0, npts);
for k = 1:2
    for i=k:2:length(pset)
        % distance matrix from this point set to all points
        r = [];
        for kk=1:2
        for j=kk:2:length(pset)
            [cx,rx]=meshgrid(pset{j}(:,1), pset{i}(:,1));
            [cy,ry]=meshgrid(pset{j}(:,2), pset{i}(:,2));
            r1 = sqrt((cx-rx).^2+(cy-ry).^2);
            r = [r r1];
        end
        end
        rt = [rt; r];

        if pset_op(i) == '0'
            A = [A; gauss(r)];
        else
            A = [A; gauss(r)];
        end
    end
end
toc
difAT = (A - tt).^2;
norm(difAT)
pause
quit()
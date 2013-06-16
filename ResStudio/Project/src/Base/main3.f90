program main3

	use class_point_set
	use matrix_class
	use fp
	use radial_basis_function
	use timer
	
	implicit none
	
	type(point_set)							:: pts
	real(kind=rfp), dimension(:,:), pointer :: pts_i, pts_j
	character(len=80)						:: file_path
	type(matrix), dimension(:,:), pointer	:: dists					! dist matrices
	integer, dimension(1:2)					:: rg, sp					! range, span
	integer									:: i, j, block_i, block_j	! loop indices
	character, dimension(:), pointer		:: op						! L or 0
	
	! dphi anrop
	character(len=80)						:: phi
	character								:: nprime
	integer									:: nd
	real(kind=rfp)							:: eps
	real(kind=rfp), dimension(:,:), pointer :: dist, distT, block
	
	! resultat
	real(kind=rfp), dimension(:,:), pointer	:: res						! result matrix
	integer									:: offset_i, offset_j		! blk offset i res
	
	! timing
	integer(kind=8)							:: time
	
	file_path = "input/boxgeom256.dat"
	allocate(op(1:8))
	op = (/"0","0","0","0","L","L","L","L"/)
	
	! file_path = "input/new_geom.dat"
	! allocate(op(1:10))
	! op = (/"0","0","0","0","0","0","L","L","L","L"/)
	
	pts = new_pts(file_path)
	
	! allocate result matrix
	rg = range_pts(pts)
	allocate(res(1:rg(2),1:rg(2)))
	
	! hämta span av domänerna i pointset
	sp = span_pts_id(pts)
	
	! allocate distance matrix matrix
	! allocate(dists(1:sp(2)-sp(1)+1, 1:sp(2)-sp(1)+1))
	allocate(dists(sp(1):sp(2), sp(1):sp(2)))
	
	! write(*,*) "sp = ", sp(1), sp(2)
	! write(*,*) "dist size = ", size(dists,1), size(dists,2)
	
	! write(*,*) "dist"
	
	time = getime()
	
	! övertriangulär
	! for pair of two domains do calc distance matrix
	! do block_i=0,0
		! do block_j=0,0
	do block_i=sp(1),sp(2)
		do block_j=block_i,sp(2)

			! write(*,*) block_i, block_j

			! for each pair of points in the domain pair calc distance
			pts_i => get_pts_ptr(block_i, pts)
			pts_j => get_pts_ptr(block_j, pts)
		
			! write(*,*) 1
			! write(*,*) "dist", 1, ":", size(pts_i, 1), "x", 1, ":", size(pts_j, 1)
		
			allocate(dists(block_i,block_j)%grid(1:size(pts_i, 1), 1:size(pts_j, 1)))
			do i=1,size(dists(block_i,block_j)%grid,1)
				do j=1,size(dists(block_i,block_j)%grid,2)
					! write(*,*) i, j
					dists(block_i,block_j)%grid(i,j) = sqrt((pts_j(j,1)-pts_i(i,1))**2+(pts_j(j,2)-pts_i(i,2))**2)
				end do
			end do
			
			! write(*,*) 2
			
			! if(block_i == 0 .and. block_j == 1) then
				! write(*,*) "dists(", block_i, ",", block_j ,")=[", dists(block_i,block_j)%grid, "]"
			! end if
			! write(*,*) 3
			
		end do
	end do
	
	! write(*,*) "/dist"
	
	! write(*,*) "dhpi"
	
	! calc dphi
	do block_i=sp(1),sp(2)
		do block_j=sp(1),sp(2)
		
			! write(*,*) "----------"
			! write(*,*) "dphi", block_i, block_j

		
			! allocate block
			pts_i => get_pts_ptr(block_i, pts)
			pts_j => get_pts_ptr(block_j, pts)
			allocate(block(1:size(pts_i, 1), 1:size(pts_j, 1)))
			
			! sätt argument
			phi = "gauss"
			nprime = op((block_i-sp(1))+1)
			nd = dims_pts(pts)
			eps = 2
			! beräkna block
			if(block_j >= block_i) then ! distansmatriserna hade övertriangulära namn
				dist => dists(block_i, block_j)%grid
				! write(*,*) "no transpose"
				! write(*,*) trim(phi), " ", nprime, nd, eps
				! write(*,*) dist(1,1), dist(1,2)
				! write(*,*) "dist=[", dist, "]"
				! write(*,*) "block=[", block, "]"
				call dphi(phi, nprime, nd, eps, dist, block)
			else
				dist => dists(block_j, block_i)%grid
				! write(*,*) "transpose"
				! write(*,*) trim(phi), " ", nprime, nd, eps
				! write(*,*) "allocating: ", size(dist, 2), "x" , size(dist, 1)
				allocate(distT(1:size(dist, 2), 1:size(dist, 1)))
				distT = transpose(dist)
				call dphi(phi, nprime, nd, eps, distT, block)
				deallocate(distT)
			end if
				
			! beräkna offset i resultat matris
			rg = range_pts(block_i, pts)
			offset_i = rg(1)-1
			rg = range_pts(block_j, pts)
			offset_j = rg(1)-1
					
			! sätt in i resultat matris
			do i=1,size(block,1)
				do j=1,size(block,2)
					res(offset_i+i, offset_j+j) = block(i,j)
				end do
			end do
		
			! deallocate block
			deallocate(block)
			
			! write(*,*) "----------"
				
		end do
	end do

	! write(*,*) "/dphi"
	
	! write(*,*) "Output"
	
	! open(unit=42, file="output/output_serial.dat", action="write", status="replace")
	! do i=1,size(res,1)
		! write(42,*) (res(i,j),j=1,size(res,2)) 
	! end do
	
	time = getime()-time
	
	write(*,*) "Done. ", time
	
end program main3










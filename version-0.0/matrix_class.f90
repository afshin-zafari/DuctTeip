module matrix_class

	use fp

Public matrix

	type matrix
        
    Real(kind=rfp), Pointer, Dimension(:,:) :: grid
	end type matrix
 
contains

end module matrix_class

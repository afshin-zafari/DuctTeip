# 1 "approximation_class.F90"
# 1 "<command-line>"
# 1 "approximation_class.F90"






# 1 "debug.h" 1



# 24 "debug.h"



                          
# 36 "debug.h"

# 46 "debug.h"

# 56 "debug.h"

# 66 "debug.h"

# 76 "debug.h"

# 86 "debug.h"

# 7 "approximation_class.F90" 2


Module approximation_class

	Use class_epsilon
	Use class_point_set

	Implicit None

!    public :: approximation_new
	Type approximation
		Character(len=80)			:: phi
		Type(epsilon)				:: eps

		Type(point_set), Pointer	:: pts,pts2

	End Type approximation


 !   interface approximation_new
 !       module procedure approximation_new,approximation_new2
 !   end interface
 
 Contains 


    Subroutine approximation_new2(this, phi, eps,pts,pts2)

      Type(approximation)                   , Intent(inout)  :: this
      Character(len=*)                      , Intent(in)     :: phi
      Type(epsilon)                         , Intent(in)     :: eps
      Type(point_set)                       , Intent(in)     :: pts,pts2
      Integer           ,Dimension(:,:)     , Pointer        :: ids
      Integer 						     :: i ,l
      
      
      this%phi = phi(1:80)
      
      this%eps = eps
      
      Allocate(this%pts)
      Allocate(this%pts2)
      
      this%pts = pts
      this%pts2 = pts2
      

    End Subroutine approximation_new2
    
    Function approximation_get_point_set2(this) Result(pts)
      Type(approximation), Intent(in)	:: this
      Type(point_set), Pointer		:: pts
      pts => this%pts2
    End Function approximation_get_point_set2




	Subroutine approximation_new(this, phi, eps, file_path)
		Type(approximation), Intent(inout)	:: this
		Character(len=*), Intent(in)		:: phi, file_path
		Type(epsilon), Intent(in)			:: eps
		
		this%phi = phi
		this%eps = eps
		Allocate(this%pts)
		this%pts = new_pts(file_path)
		
	End Subroutine approximation_new

	! TODO approximation_delete
	
	Function approximation_get_point_set(this) Result(pts)
		Type(approximation), Intent(in)	:: this
		Type(point_set), Pointer		:: pts
		pts => this%pts
	End Function approximation_get_point_set

	
	Function approximation_get_phi(this) Result(phi)
		Type(approximation), Intent(in)	:: this
		Character(len=80) :: phi
		phi = this%phi
	End Function approximation_get_phi
	
	Function approximation_get_eps(this) Result(eps)
		Type(approximation), Intent(in)	:: this
		Type(epsilon)					:: eps
		eps = this%eps
	End Function approximation_get_eps
	
End Module approximation_class

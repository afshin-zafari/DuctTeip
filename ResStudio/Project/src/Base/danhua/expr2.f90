! $$$ expr2.f90 $$$
! single 'op, coeff_op, coeff_fun'

! class for expression
!


!$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
! There is a problem about term_number, it can vary for different equations
! but now it's a matrix, probably it should introduce 'block' to represent one equqtion
!$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$


module class_expression

use fp
use class_field
use class_function_value
!
implicit none
private
!
public :: expression
public :: new_expr, add_term, get_coeff, destruct_expr
public :: get_eq_num, get_term_num

interface add_term
	module procedure add_term_r, add_term_l
end interface

interface new_term
	module procedure new_term_r, new_term_l
end interface

!
!
!
type term 
  ! integer, dimension(:), pointer :: dim, prime  
  character(len=80):: op, op_c, coeff_f              ! operator, op_of_coeff and coeff_function                                  
  character(len=80), dimension(:), pointer :: fun      ! functions for rhs              
  logical :: cfn, lr , reel                       !True: coefficent function, right side, real type
  real(kind=rfp) :: co_r               ! for real coeff, if with coeff_fun, it could be as sign(+/-1.0)
  complex(kind=cfp) :: co_c            ! for complex coeff
  !type(field) :: coeff  
end type term
!
!

type expression
  private
  integer:: neq
  integer, dimension(:), pointer :: id , nterm        ! each equation relate to which Geometry_ojbect
	type(term), dimension(:,:), pointer :: eqs	
end type expression
!

contains               ! functions
!
!$$$$$$$$$$$$$ constructor $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
!
function new_expr(nterm) result(pde)
!integer, dimension(1:), intent(in):: nterm

integer, dimension(1:2), intent(in):: nterm
integer :: i,j
type(expression) :: pde
!
	i = nterm(1)
	j = nterm(2)
	allocate(pde%eqs(1:i, 1:j))
	allocate(pde%id(1:i), pde%nterm(1:i))
	pde%neq = 0
	pde%nterm = 0
	
!  do k =1,ubound(nterm,1)
!		allocate(pde%eqs(k, 1:nterm(k)))
!	end do
		
end function new_expr
!
!
!
!  $$$$$$$$$$ create terms $$$$$$$$$$$$$$$$$$$$
!


Function new_term_r(fun,op,rs,rval,cval) Result(t)
  Character(len=*), Dimension(1:), Intent(in):: fun           ! function_f
character(len=*), intent(in) :: op              ! operator of term
logical, intent(in) :: rs                        ! with or without u(right side)
real(kind=rfp),   optional :: rval         !coeff_value: real
complex(kind=cfp), optional :: cval      !coeff_value: complex
!
type(term) :: t
!
integer ::  nfun                 ! number

!  	
	 nfun = ubound(fun,1)    !! number of function_f
  allocate(t%fun(1:nfun))  
  
   t%op = op
   t%fun =  fun 
   t%lr = rs   			
   t%cfn = .false.              ! coefficient is constant
    				
  if(present(rval)) then              
 		t%reel = .true. 	      		    			
    t%co_r = rval
	else if(present(cval)) then
		t%reel = .false.
		t%co_c = cval 
	else 
		write(*,*) 'Error: Define type of coeff in call to new_term' 
 	end if
 
end function new_term_r
!
!
! function new_term_r(op,fun,rs,rval,cval) result(t)

function new_term_l(op,rs,rval,cval,cfun,cop) result(t)
character(len=*), intent(in) :: op              ! operator of term
logical, intent(in) :: rs                       ! with or without u(right side)
real(kind=rfp),  optional :: rval               !coeff_value: real
complex(kind=cfp), optional :: cval             !coeff_value: complex
character(len=*), optional ::  cfun, cop        ! coeff_fun, coeff_operator
type(term) :: t

!  	
   t%op = op
   t%lr = rs 
   
  if(present(rval)) then              
 		t%reel = .true. 	      		    			
    t%co_r = rval
	else if(present(cval)) then
		t%reel = .false.
		t%co_c = cval 
	else 
		write(*,*) 'Error: Define type of coeff in call to new_term' 
 	end if  
    			
  t%cfn = .false.              ! coefficient is constant
  if(present(cfun) .and. present(cop)) then
	 	t%cfn = .true.    			   ! coefficient function	   				
  	t%op_c = cop
		t%coeff_f = cfun
	else if(present(cfun) .or. present(cop)) then
		write(*,*) 'Error: Should define both coeff_fun and coeff_operator'
	end if
 
	nullify(t%fun)
end function new_term_l




!
!$$$$$$$$$$$$$ add term without u $$$$$$$$$$$$$$$$$$$$$$$$$$$
!
subroutine  add_term_r(pde, eq, term, fun, op, coeff_r, coeff_c)
type(expression), intent(inout) :: pde
integer, intent(in) :: eq , term              ! which equation ! which term, first or second
character(len=*), dimension(:), intent(in) :: fun
character(len=*), optional :: op              ! operator of fun
real(kind=rfp),  optional :: coeff_r          !real data type
complex(kind=cfp), optional :: coeff_c       !complex data type

logical :: rs          ! right side 
character(len=80) :: op_f        ! operator of fun

	write(*,*) "add_term_r"

	rs = .true.
	if(present(op)) then
		if(present(coeff_r)) then		
			pde%eqs(eq, term) = new_term(fun, op, rs, coeff_r)     	  !op is the operator of function_f			
		else if(present(coeff_c)) then
			pde%eqs(eq, term) = new_term(fun, op, rs, cval=coeff_c)
		else
			write(*,*) 'Error: Define type of coeff in call to new_term'
			stop
		end if
	else                          ! without 'op_of_f', defalut is '0'
		op_f = '0'                              ! operator of fun
		if(present(coeff_r)) then			
			pde%eqs(eq, term) = new_term(fun, op_f, rs, coeff_r)
		else if(present(coeff_c)) then		
			pde%eqs(eq, term) = new_term(fun, op_f, rs, cval=coeff_c)
		else
			write(*,*) 'Error: Define type of coeff in call to new_term'
			stop
		end if
	end if
	
	pde%nterm(eq) = pde%nterm(eq) + 1             !  term number add one
end subroutine add_term_r
!
!subroutine  add_term_r(pde, eq, term, fun, op, coeff_r, coeff_c)
!$$$$$$$$$$$$$ add term with u $$$$$$$$$$$$$$$$$$$$$$$$$$$
!
subroutine  add_term_l(pde, eq, term, op, coeff_r, coeff_c, coeff_f, op_co)
type(expression), intent(inout) :: pde
integer, intent(in) :: eq , term               ! which equation, which term, first or second
character(len=*), intent(in) :: op             !operators of the term
real(kind=rfp),  optional :: coeff_r           !real data type : for sign(+/-)
complex(kind=cfp), optional :: coeff_c         !complex data type: for sign(+/-)
character(len=*), optional :: coeff_f, op_co   !coeff_function, coeff_operator

logical :: rs          ! right side 
character(len=80) :: op_c

	write(*,*) "add_term_l"

	rs = .false.
	if(present(coeff_r)) then
		if(present(coeff_f)) then
			if(present(op_co)) then
				pde%eqs(eq, term) = new_term_l(op, rs, coeff_r, cfun=coeff_f, cop=op_co)
			else
				op_c = '0'
   			pde%eqs(eq, term) = new_term_l(op, rs, coeff_r, cfun=coeff_f, cop=op_c)
			end if
		else
			pde%eqs(eq, term) = new_term_l(op,  rs, coeff_r)
		end if
	else if(present(coeff_c)) then
		if(present(coeff_f)) then
			if(present(op_co)) then
				pde%eqs(eq, term) = new_term_l(op, rs, cval=coeff_c, cfun=coeff_f, cop=op_co)
			else
				op_c = '0'
   			pde%eqs(eq, term) = new_term_l(op, rs, cval=coeff_c, cfun=coeff_f, cop=op_c)
			end if
		else
			pde%eqs(eq, term) = new_term_l(op, rs, cval=coeff_c)
		end if
	else
		write(*,*) 'Error: Define type of coeff in call to add_term'
		stop
	end if
	
	pde%nterm(eq) = pde%nterm(eq) + 1             !  term number add one
end subroutine add_term_l




! get the number of equations for one module
function get_eq_num(pde) result(neq)
type(expression), intent(in) :: pde
integer :: neq
!integer, intent(out) :: neq

!	  neq = pde%neq                       ! where to add equation number??????????????
    neq = ubound(pde%eqs,1)   
end function get_eq_num

! get the number of terms in one equation
function get_term_num(pde,eq) result(nterm)
type(expression), intent(in) :: pde
integer, intent(in) :: eq 
integer :: nterm

	!nterm = size(pde%eqs(eq,:))	
	nterm = pde%nterm(eq)        
end function get_term_num


!
! $$$$$$$$ get coefficient $$$$$$$$$$$$$$$$$$$$
!a term with only one coeff_func and coeff_op
!could get coeffs for many points at different times
!
function get_coeff(pde,eq,term,xk,t) result(cv)
type(expression), intent(in) :: pde
integer, intent(in) :: eq, term
real(kind=rfp), dimension(1:,1:), intent(in) :: xk                ! data type ??????????????
real(kind=rfp), dimension(1:), optional :: t

type(field) :: cv                                               !coeff value  !data type ???????  

real(kind=rfp), dimension(:), pointer :: fp_r         
complex(kind=cfp), dimension(:), pointer :: fp_c 
real(kind=rfp), dimension(:,:), pointer :: fp_r2         
complex(kind=cfp), dimension(:,:), pointer :: fp_c2 
character(len=80) :: func, nprime

integer :: neq, nterm, nt, k
logical :: reel



	neq = get_eq_num(pde)
	nterm = get_term_num(pde, eq)
	
	if(eq > neq .or. term > nterm) then
		write(*,*) 'Error: This term is not existed.'
	else 
			reel = pde%eqs(eq, term)%reel                  ! real or complex 
			if(present(t)) then                            ! if time-dependent
				nt = ubound(t,1)
				cv = new_field(reel, ubound(xk,1), nt)
			else
				cv = new_field(reel, ubound(xk,1))
			end if
			
    	if(pde%eqs(eq, term)%cfn) then                 ! coefficient function    	
				func = pde%eqs(eq, term)%coeff_f
				nprime = pde%eqs(eq, term)%op_c
		
				if(reel) then               ! real type
					if(present(t)) then         !with time
						fp_r2 => cv%r%r2
						do k=1,nt
						  write(*,*) 'Call evalf() with t'        ! have no this function in fval.f90 now
							!call evalf(func, nprime, xk, t(k), fp_r2(:,k),)   !! one function's values stored in one colum of fp_r2(:,:)  
						end do
						fp_r2 = fp_r2 * pde%eqs(eq, term)%co_r
					else 										   !without time
						fp_r => cv%r%r1
     				call evalf(func, nprime, xk, fp_r)   !! one function's values stored in one colum of fp_r(:,:)  
      			fp_r = fp_r * pde%eqs(eq, term)%co_r
      		end if
      		
      	else                       !complex type
      		if(present(t)) then        !with time
						fp_c2 => cv%c%c2
						do k=1,nt
							write(*,*) 'Call evalf() with t'
							!call evalf(func, nprime, xk, t(k), fp_c2(:,k),)   !! one function's values stored in one colum of fp_r(:,:)  
						end do
						fp_c2 = fp_c2 * pde%eqs(eq, term)%co_c
					else       	                 !without time            
						fp_c => cv%c%c1
						call evalf(func, nprime, xk, fp_c)   
      			fp_c = fp_c * pde%eqs(eq, term)%co_c
      		end if        
      		
      	end if          ! if(reel)
  	
      else                              ! coefficient: constant
      	if(reel) then 
      		if(present(t)) then
      			cv%r%r2 = pde%eqs(eq, term)%co_r
      		else
      			cv%r%r1 = pde%eqs(eq, term)%co_r
      		end if
      	else
      		if(present(t)) then
      			cv%c%c2 = pde%eqs(eq, term)%co_c
      		else
      	 		cv%c%c1 = pde%eqs(eq, term)%co_c
      	 	end if    
   			end if       !	if(reel)
   		end if        ! if(pde%eqs(eq, term)%cfn)
   		
   	end if

end function get_coeff



!
subroutine destruct_term(t)
type(term) :: t
!
  if(associated(t%fun)) then
  	deallocate(t%fun)
  end if
!
end subroutine destruct_term
!


subroutine destruct_expr(pde)
type(expression) :: pde
!
integer :: k, j
!
  do k=1,ubound(pde%eqs,1)
  	do j=1,size(pde%eqs(k,:))
    	call destruct_term(pde%eqs(k,j))
    end do
  end do
  deallocate(pde%id)
  deallocate(pde%nterm)
end subroutine destruct_expr


end module class_expression


! $$$ fval.f90 $$$
!
module class_function_value
!
!x,y,z..are different colums in the matrix.
!funtions' value F is 1D vector(i.e. one colum of matrix)
!
! Functions that are defined:
!   'const'     F = 1
!   'p1'        F = 1 +2x -3y +4z + ...
!   'p2'        F = 1+x+x^2 -xy-2y+2y^2 +3z+3z^2 + ...
!   'r2'        F = r^2
!   'r4'        F = r^4
!   'xy3'       F = xy^3
!   'x5y3'      F = x^5y^3
!   'card'      F = 1 at the first point 0 at the rest
!   'fdwc1'     F = 25/(25 + (x-0.2)^2 + 2y^2)
!   'rat1'      F = 25/(25 + (x-0.2)^2 + 2(y+0.1)^2)
!   'rat2'      F =  1/( 1 + (x - 1)^2 + 2y^2)
!   'rat3'      F = 65/(65 + (x-0.2)^2 + (y+0.1)^2)
!   'rat4'      F = 165/(165 + (x-0.2)^3 + 2(y+0.1)^3)
!   'rat5'      F = 165/(165 + x + 2y)
!   'exp1'      F = exp(-g), g = (x-0.1)^2 + 0.5y^2
!   'exp2'      F = exp(g)/exp(1.21)
!   'gauss'     F = exp(-delta^2*r^2)
!   'sin'       F = 0.5*sin(x-y)
!   'sin2pi'    F = sin(2*pi*x)
!   'trig1'     F = 0.5*sin(0.5x+y) + 0.5*cos(2x+0.25y)
!   'trig2'     F = 0.5(sin(x) + cos(2x))
!   'trig3'     F = sin(pi*r^2)
!   'lap1'      F = (1 - 0.5r*sin(theta) + 0.25r^2*cos(2*theta))/1.375
!   'lap2'      F = (r^3*sin(3*theta) - 0.1r^4*cos(4*theta))/1.1
!   'fdwc2'     F = atan(2*(x+3y-1))/maximum
!
! For Dam Seapage
!   'head25'    F = 25
!   'head35'    F = 35

!   Some of the functions are scaled to have maximum value 1.
!
! Operations that are supported:
!   '0'         The function value
!   'L'         The Laplacian of the function
!
use fp
use const
use eqconst
use output
use class_field, &
    function_value => field, destruct_fval => destruct_field
use class_point_set
!
implicit none
!
private
!
public :: function_value, new_fval, destruct_fval, evalf, write_fval
!
interface evalf               ! evaluation function
  module procedure evalf_r, evalf_c
end interface
!
contains
!
! get functions 'funcs' value based on point_set 'x', return in 'F'(one colum for each function)
!
  function new_fval(funcs,x) result(F)
  character(len=*), dimension(1:), intent(in) :: funcs
  type(point_set), intent(in) :: x 
!
  type(function_value) :: F        !! == type_field
!
  logical :: reel                  !! decide which field type
  character :: nprime              !! operation's type: for function's value or Laplacian of the function
  integer :: k,nrhs
  integer, dimension(1:2) :: rg    !! range of point sets
  real(kind=rfp), dimension(:,:), pointer :: xp,fp_r
  complex(kind=cfp), dimension(:,:), pointer :: fp_c 
!
    nrhs = ubound(funcs,1)         !! functions' number
    rg = range_pts(x)              !! points' 1D rows in the matrix
    ! 
    ! Real or complex depends on which functions we are asking for
    !
    reel = .true.                  !! type real
    do k=1,nrhs
      if (trim(funcs(k))=='helm1'.or. trim(funcs(k))=='helm2' &
                                 .or. trim(funcs(k))=='helmx') reel=.false.
    end do
    F = new_field(reel,rg(2),nrhs)
    nprime = '0'                   !! compute function's value
    ! 
    xp => get_pts_ptr(x)           !! xp points to inputed point_set x
    if (reel) then
      fp_r => F%r%r2               !!fp_r points to 2D real field
      do k=1,nrhs
        call evalf(funcs(k),nprime,xp,fp_r(:,k))   !! one function's values stored in one colum of fp_r(:,:)
      end do
    else
      fp_c => F%c%c2
      do k=1,nrhs
        call evalf(funcs(k),nprime,xp,fp_c(:,k))
      end do
    end if
!
  end function new_fval
!
!
  subroutine write_fval(f,fname)
  type(function_value), intent(in) :: f
  character(len=*), intent(in), optional :: fname
!
  logical :: re               ! detify data type
  integer :: nrhs, nf, j,k
!
    if (associated(f%r%r2)) then
      re = .true.
      nf = ubound(f%r%r2,1)     ! row number
      nrhs = ubound(f%r%r2,2)   ! colum number
    else
      re = .false.
      nf = ubound(f%c%c2,1)
      nrhs = ubound(f%c%c2,2)
    end if
!
    if (present(fname)) then      ! if file exist
      if (re) then
        call write_to_file(fname,f%r%r2)
      else
        call write_to_file(fname,f%c%c2)
      end if
    else                          !if file do not exist
      write(*,*)
      do k=1,nrhs
        write(*,*) 'Values for function',k
        if (re) then
          do j=1,nf
            write(*,*) f%r%r2(j,k)
          end do
        else
          do j=1,nf
            write(*,*) real(f%c%c2(j,k)), aimag(f%c%c2(j,k))
          end do
        end if
      end do
    end if
  end subroutine write_fval
!
!
  subroutine evalf_r(func,nprime,x,F)
  character(len=*), intent(in) :: func,nprime
  real(kind=rfp), dimension(1:,1:), intent(in) :: x
  real(kind=rfp), dimension(1:), intent(out) :: F
!
  integer :: nd,k
  real(kind=rfp), dimension(1:ubound(F,1)) :: theta,r
! 
    nd = ubound(x,2)
    !---------------------------------------------------
    ! Polynomial functions
    !---------------------------------------------------
    if (trim(func)=='const') then
      F = 1.0_rfp
      if (trim(nprime)=='L') then          !!compute Laplacian of the function
        F = 0.0_rfp
      end if

!$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$  
    
    else if (trim(func)=='head25') then
      F = 25.0_rfp
      if (trim(nprime)=='L') then          
        F = 0.0_rfp
      end if
      
   else if (trim(func)=='head35') then
        if (trim(nprime)=='0') then 
          F = 35.0_rfp
        else if (trim(nprime)=='L') then          
          F = 0.0_rfp
        else if (trim(nprime)=='Y') then 
          F = x(:,2)
        end if
      
!
!$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ 
! 
    else if (trim(func)=='p1') then
      if (trim(nprime)=='0') then
        F = 1.0_rfp + 2.0_rfp*x(:,1)
        do k=2, nd
          F = F + (-1)**(k-1)*(k+1)*x(:,k)
        end do
      else if (trim(nprime)=='L') then
          F = 0.0_rfp
      end if
!
    else if (trim(func)=='p2') then
      if (trim(nprime)=='0') then
        ! r^2 terms
        F = x(:,1)**2.0_rfp                !! F = x^2
        do k=2, nd
          F = F + k*x(:,k)**2.0_rfp        !!! F = x^2 + y^2 + z^2....
        end do
        ! Constant and linear terms
        F = F + 1.0_rfp + x(:,1) 
        do k=2, nd
          F = F + (-1)**(k-1)*k*x(:,k)
        end do
        ! If 2D, add the crossproduct term
        if (nd==2) then
          F = F - x(:,1)*x(:,2)
        end if
      else if (trim(nprime)=='L') then   !! F = 2+4+6+8....
        F = 2.0_rfp
        do k=2,nd
          F = F + 2.0_rfp*k
        end do
      end if
!
    else if (trim(func)=='r2') then
      if (trim(nprime)=='0') then
        F = x(:,1)**2.0_rfp
        do k=2, nd
          F = F + x(:,k)**2.0_rfp
        end do
      else if (trim(nprime)=='L') then
        F = 2.0_rfp*nd
      end if
!
    else if (trim(func)=='r4') then
      ! First r^2 is computed
      F = x(:,1)**2.0_rfp
      do k=2, nd
        F = F + x(:,k)**2.0_rfp
      end do
      !
      if (trim(nprime)=='0') then
        F = F**2.0_rfp
      else if (trim(nprime)=='L') then
        F = 16.0_rfp*F
      end if
!
    else if (trim(func)=='xy3') then
      if (trim(nprime)=='0') then
        F = x(:,1)*x(:,2)**3.0_rfp
      else if (trim(nprime)=='L') then
        F = 6.0_rfp*x(:,1)*x(:,2)
      end if
!
    else if (trim(func)=='x5y3') then
      !
      if (trim(nprime)=='0') then
        F = x(:,1)**5.0_rfp*x(:,2)**3.0_rfp
      else if (trim(nprime)=='L') then
        F = 20.0_rfp*(x(:,1)*x(:,2))**3.0_rfp
        F = F + 6.0_rfp*x(:,1)**5.0_rfp*x(:,2)
      end if
    !---------------------------------------------------
    ! Cardinal function
    !---------------------------------------------------
    else if (trim(func)=='card') then
      if (trim(nprime)=='0') then
        F = 0.0_rfp
        F(1) = 1.0_rfp 
      else if (trim(nprime)=='L') then
        write(*,*) 'Error: Derivatives of cardinal functions are unknown'
        stop
      end if
    !---------------------------------------------------
    ! Rational functions 
    !---------------------------------------------------
    else if (trim(func)=='fdwc1') then
      F = 25.0_rfp + (x(:,1)-0.2_rfp)**2.0_rfp 
      if (nd >= 2) then
        F = F + 2.0_rfp*x(:,2)**2.0_rfp
      end if
      F = 1.0_rfp/F
      if (trim(nprime)=='L') then
        if (nd==1) then
          F = 8.0_rfp*(x(:,1)-0.2_rfp)**2.0_rfp*F**3.0_rfp -  &
              2.0_rfp*F**2.0_rfp
        else
          F = (8.0_rfp*(x(:,1)-0.2_rfp)**2.0_rfp + &
              32.0_rfp*x(:,2)**2.0_rfp)*F**3.0_rfp - 6.0_rfp*F**2.0_rfp 
        end if
      end if
      F = 25.0_rfp*F 
!
    else if (trim(func)=='rat1') then
      if (nd==1) then
        write(*,*) 'For 1D-problems, use fdwc1 instead of rat1'
        stop
      end if
      F = 25.0_rfp + (x(:,1)-0.2_rfp)**2.0_rfp & 
          + 2.0_rfp*(x(:,2)+0.1_rfp)**2.0_rfp
      F = 1.0_rfp/F
      if (trim(nprime)=='L') then
        F = (8.0_rfp*(x(:,1)-0.2_rfp)**2.0_rfp  &
             +32.0_rfp*(x(:,2)+0.1_rfp)**2.0_rfp)*F**3.0_rfp &
             -6.0_rfp*F**2.0_rfp 
      end if
      F = 25.0_rfp*F 
!
    else if (trim(func)=='rat2') then
      if (nd==1) then
        write(*,*) 'For 1D-problems, use fdwc1 instead of rat2'
        stop
      end if      
      F = 1.0_rfp + (x(:,1)-1.0_rfp)**2.0_rfp + 2.0_rfp*x(:,2)**2.0_rfp
      F = 1.0_rfp/F
      if (trim(nprime)=='L') then
        F = (8.0_rfp*(x(:,1)-1.0_rfp)**2.0_rfp + &
            32.0_rfp*x(:,2)**2.0_rfp)*F**3.0_rfp - 6.0_rfp*F**2.0_rfp 
      end if
!
    else if (trim(func)=='rat3') then
      if (nd==1) then
         F = 1.0_rfp/(65.0_rfp+(x(:,1)-0.2_rfp)**2.0_rfp)
         if (trim(nprime)=='L') then
            F = 8.0_rfp*(x(:,1)-0.2_rfp)**2.0_rfp*F**3.0_rfp &
                 -2.0_rfp*F**2.0_rfp
         end if
      else
        F = 1.0_rfp/(65+(x(:,1)-0.2_rfp)**2.0_rfp+(x(:,2)+0.1_rfp)**2.0_rfp)
        if (trim(nprime)=='L') then
          F = 8.0_rfp*((x(:,1)-0.2_rfp)**2.0_rfp + &
               (x(:,2)+0.1_rfp)**2.0_rfp)*F**3.0_rfp &
               -4.0_rfp*F**2.0_rfp
        end if
      end if
      F = F*65.0_rfp
!
    else if (trim(func)=='rat4') then
      if (nd==1) then
        write(*,*) 'rat4 is not implemented for 1D-problems'
        stop
      end if
      F = 1.0_rfp/(165+(x(:,1)-0.2_rfp)**3.0_rfp &
          +2.0_rfp*(x(:,2)+0.1_rfp)**3.0_rfp)
      if (trim(nprime)=='L') then
        F = (18.0_rfp*(x(:,1)-0.2_rfp)**4.0_rfp + &
             72.0_rfp*(x(:,2)+0.1_rfp)**4.0_rfp)*F**3.0_rfp
        F = F - (6.0_rfp*(x(:,1)-0.2_rfp) + &
             12.0_rfp*(x(:,2)+0.1_rfp))*F**2.0_rfp
      end if
      F = F*165.0_rfp

    else if (trim(func)=='rat5') then
      if (nd==1) then
        F = 1.0_rfp/(165+x(:,1))
        if (trim(nprime)=='L') then
          F = 2.0_rfp*F**3.0_rfp
        end if
      else
        F = 1.0_rfp/(165+x(:,1)+2.0_rfp*x(:,2))
        if (trim(nprime)=='L') then
          F = 10.0_rfp*F**3.0_rfp
        end if
      end if
      F = F*165.0_rfp
    !---------------------------------------------------
    ! Exponential functions 
    !---------------------------------------------------
    else if (trim(func)=='exp1') then
      if (nd==1) then
        F = (x(:,1)-0.1_rfp)**2.0_rfp
        F = exp(-F)
        if (trim(nprime)=='L') then
          F = (-2.0_rfp+4.0_rfp*(x(:,1)-0.1)**2.0_rfp)*F 
        end if
      else
        F = (x(:,1)-0.1_rfp)**2.0_rfp + 0.5*x(:,2)**2.0_rfp
        F = exp(-F)
        if (trim(nprime)=='L') then
          F = (-3.0_rfp+4.0_rfp*(x(:,1)-0.1)**2.0_rfp+x(:,2)**2.0_rfp)*F 
        end if
      end if

    else if (trim(func)=='exp2') then
      if (nd==1) then
        F = (x(:,1)-0.1_rfp)**2.0_rfp
        F = exp(F)
        if (trim(nprime)=='L') then
          F = (2.0_rfp+4.0_rfp*(x(:,1)-0.1)**2.0_rfp)*F 
        end if
      else
        F = (x(:,1)-0.1_rfp)**2.0_rfp + 0.5*x(:,2)**2.0_rfp
        F = exp(F)
        if (trim(nprime)=='L') then
          F = (3.0_rfp+4.0_rfp*(x(:,1)-0.1)**2.0_rfp+x(:,2)**2.0_rfp)*F 
        end if
      end if
      F = F/exp(1.21_rfp)
!     Maximum at x=-1 y=0

    else if (trim(func)=='gauss') then
        F = x(:,1)**2.0_rfp
        do k=2, nd
          F = F + x(:,k)**2.0_rfp
        end do
        F = exp(-gdel*F)

    else if (trim(func)=='peaks') then
        F = exp(-9.0_rfp*((x(:,1)-0.28_rfp)**2.0_rfp + &
             (x(:,2)-0.59_rfp)**2.0_rfp))
        F = F + 0.5_rfp*exp(-9.0_rfp*((x(:,1)+0.37_rfp)**2.0_rfp + &
             (x(:,2)-0.04_rfp)**2.0_rfp))
        F = F - 0.25_rfp*exp(-9.0_rfp*((x(:,1)+0.57_rfp)**2.0_rfp+ &
             (x(:,2)+0.7_rfp)**2.0_rfp))
        F = F + 0.34*exp(-9.0_rfp*((x(:,1)-0.29_rfp)**2.0_rfp + &
             (x(:,2)+0.24_rfp)**2.0_rfp))

    else if (trim(func)=='wend6') then
        r = sqrt((x(:,1)-0.3_rfp)**2.0_rfp+(x(:,2)-0.5_rfp)**2.0_rfp)
        F = (1.0_rfp-r)**2.0_rfp
        F = F*F
        F = F*F
        F = F*(32.0_rfp*r**3.0_rfp+25.0_rfp*r**2.0_rfp+8.0_rfp*r+1.0_rfp)
        where (r > 1.0_rfp) 
          F = 0.0_rfp
        end where

    !---------------------------------------------------
    ! Trigonometric functions 
    !---------------------------------------------------
    else if (trim(func)=='sin') then
      F = x(:,1)
      if (nd>=2) then
        F = F - x(:,2)
      end if
      F = 0.5_rfp*(sin(F))
      if (trim(nprime)=='L') then
        F = -F
        if (nd>=2) then
          F = 2*F
        end if 
      end if
!
    else if (trim(func)=='sin2pi') then
      F = x(:,1)
      if (nd>=2) then
        F = F - x(:,2)
      end if
      F = sin(2*pi*F)
      if (trim(nprime)=='L') then
        F = -4*pi**2.0_rfp*F
      end if
!
    else if (trim(func)=='trig1') then
      if (nd==1) then
        F = pi*x(:,1)
        if (trim(nprime)=='0') then
          F = 0.5_rfp*(sin(F)+cos(2*F))
        else if (trim(nprime)=='L') then
          F = -pi**2.0_rfp*(0.5_rfp*sin(F)+2.0_rfp*cos(2*F))
        end if
      else
        if (trim(nprime)=='0') then
          F = 0.5_rfp*sin(0.5_rfp*x(:,1)+x(:,2))
          F = F + 0.5_rfp*cos(2.0_rfp*x(:,1)+0.25*x(:,2))
        else if (trim(nprime)=='L') then
          F = -0.625_rfp*sin(0.5_rfp*x(:,1)+x(:,2))
          F = F - 2.03125_rfp*cos(2.0_rfp*x(:,1)+0.25*x(:,2))
        end if
      end if
!
    else if (trim(func)=='trig2') then
      if (nd>1) then
        write(*,*)'WARNING: trig2 is a 1D-function'
      end if
      F = x(:,1)
      if (trim(nprime)=='0') then
        F = 0.5_rfp*(sin(F)+cos(2*F))
      else if (trim(nprime)=='L') then
        F = -1.0_rfp*(0.5_rfp*sin(F)+2.0_rfp*cos(2*F))
      end if
!
    else if (trim(func)=='trig3') then
      F = x(:,1)**2.0_rfp
      do k=2, nd
        F = F + x(:,k)**2.0_rfp
      end do
      F = pi*F
      if (trim(nprime)=='0') then
        F = sin(F)
      else if (trim(nprime)=='L') then
        F = 2.0_rfp*pi*(nd*cos(F)-2.0_rfp*F*sin(F))
      end if

    else if (trim(func)=='trigA') then
      F = sin(2*pi*( x(:,1)**2.0_rfp + 2.0_rfp*x(:,2)**2.0_rfp))
      F = F -  sin(2*pi*( 2.0_rfp*x(:,1)**2.0_rfp + (x(:,2)-0.5_rfp)**2.0_rfp))
      F = F/2.0_rfp

    else if (trim(func)=='trigB') then
      F = sin( x(:,1)**2.0_rfp + 2.0_rfp*x(:,2)**2.0_rfp)
      F = F -  sin( 2.0_rfp*x(:,1)**2.0_rfp + (x(:,2)-0.5_rfp)**2.0_rfp)
      F = F/2.0_rfp


    !---------------------------------------------------
    ! Functions that satisfy Laplace's equation
    !---------------------------------------------------
    else if (trim(func)=='lap1') then
      if (trim(nprime)=='0') then
        !
        ! Compute polar coordinates
        !
        F = x(:,1)**2.0_rfp
        do k=2, nd
          F = F + x(:,k)**2.0_rfp
        end do
        F = sqrt(F)
        theta = 0.0_rfp
        where (F/=0) 
          theta = atan2(x(:,2),x(:,1))
        end where
        F = 1.0_rfp - 0.5_rfp*F*sin(theta) + &
                     0.25_rfp*F**2.0_rfp*cos(2.0_rfp*theta)
        ! Maximum occurs for theta = pi+pi/6
        F = F/1.375_rfp
      else if (trim(nprime)=='L') then
        F = 0.0_rfp
      end if
!      
    else if (trim(func)=='lap2') then
      if (trim(nprime)=='0') then
        !
        ! Compute polar coordinates
        !
        F = x(:,1)**2.0_rfp
        do k=2, nd
          F = F + x(:,k)**2.0_rfp
        end do
        F = sqrt(F)
        theta = 0.0_rfp
        where (F/=0)
          theta = atan2(x(:,2),x(:,1))
        end where
        F = 1.0_rfp*F**3.0_rfp*sin(3.0_rfp*theta) - &
                     0.1_rfp*F**4.0_rfp*cos(4.0_rfp*theta)
        ! Maximum occurs for theta=pi/2
        F = F/1.1_rfp
      else if (trim(nprime)=='L') then
        F = 0.0_rfp
      end if
    !-----------------------------------------------------
    ! Arctan function with sharp gradient on the unit disc 
    !-----------------------------------------------------
    else if (trim(func)=='fdwc2') then
      if (nd==1) then
        F = 3.0_rfp*x(:,1)-1.0_rfp
        if (trim(nprime)=='0') then
          F = atan(F)
        else if (trim(nprime)=='L') then
          F = -18.0_rfp*F/(1.0_rfp+F**2.0_rfp)**2.0_rfp
        end if
      else
         F = 2.0_rfp*(x(:,1) + 3.0_rfp*x(:,2) - 1.0_rfp)
         if (trim(nprime)=='0') then
            F = atan(F)
         else if (trim(nprime)=='L') then
            F = -80.0_rfp*F/(1.0_rfp+F**2.0_rfp)**2.0_rfp
         end if
         F = F/atan(2.0_rfp*sqrt(10.0_rfp)+2.0_rfp) !Scaling to make maximum 
         !     Maximum located at x=-1/sqrt(1) y=-3/sqrt(10)    
      end if
!
    else
      write(*,*) 'Error: The function ',func,' is not implemented', &
                 ' zeros will replace the intended right hand side'
      F = 0.0_rfp
    end if
!
  end subroutine evalf_r
!
!
  subroutine evalf_c(func,nprime,x,F)
  character(len=*), intent(in) :: func,nprime
  real(kind=rfp), dimension(1:,1:), intent(in) :: x
  complex(kind=cfp), dimension(1:), intent(out) :: F
!
  integer :: nd,k
  real(kind=rfp), dimension(1:ubound(F,1)) :: Fr
  complex(kind=cfp) :: co_l,co_r
! 
    nd = ubound(x,2)
   
    if (trim(func)/='helm1' .and. trim(func)/='helm2' &
                            .and. trim(func)/='helmx') then
      call evalf_r(func,nprime,x,Fr)
      F = cmplx(Fr,0.0_rfp,cfp)
    else if (trim(func)=='helm1') then
      if (nd>1) then
        write(*,*) 'Warning: helm1 is a 1D-function'
      end if
      ! When more cases of nprime are needed, I will add them
      F = exp(im*kappa*(1.0_rfp+im*delta)*x(:,1))
      if (trim(nprime)/='0') then
        write(*,*) 'Error: helm1 is not intended for use with the elliptic',&
                   ' operators'
        stop
      end if
    else if (trim(func)=='helmx') then
      if (nd>1) then
        write(*,*) 'Warning: helmx is a 1D-function'
      end if
      co_l = (kappa1-kappa2)/(kappa1+kappa2)*exp(im*kappa1)
      co_r = 2.0_rfp*kappa1/(kappa1+kappa2)*exp(im*(kappa1-kappa2)/2.0_rfp)

      do k=1,ubound(x,1)
        if (x(k,1)<=0.5_rfp) then
          F(k) = exp(im*kappa1*x(k,1)) + co_l*exp(-im*kappa1*x(k,1))
        else
          F(k) = co_r*exp(im*kappa2*x(k,1))
        end if
      end do
      if (trim(nprime)/='0') then
        write(*,*) 'Error: helmx is not intended for use with the elliptic',&
                   ' operators'
        stop
      end if
    else
      F = sin(alpha*x(:,2))*exp(im*lambda*x(:,1))
      if (trim(nprime)/='0') then
        write(*,*) 'Error: helm2 is not intended for use with the elliptic',&
                   ' operators'
        stop
      end if
    end if 
  end subroutine evalf_c
!
end module class_function_value

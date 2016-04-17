module eqconst
use fp
use const
  ! Temporary solution for when fval needs some coefficients like kappa
  ! Note that if kappa is changed to complex then (delta>0) and
  ! kappa(1+i*delta) is used
  real(kind=rfp) :: kappa,lambda,alpha,kappa1,kappa2
  real(kind=rfp), parameter :: delta=0.0_rfp
  real(kind=rfp) :: gdel
contains
  subroutine varkappa(x,kappa)
  real(kind=rfp), dimension(1:,1:) :: x
  real(kind=rfp), dimension(1:) :: kappa
!
  integer :: k
    do k=1,ubound(x,1)
!      kappa(k) = kappa1*(1.0_rfp+0.5_rfp*sin(2*pi*x(k,1)))  
!      kappa(k) = kappa1*(1.0_rfp+0.5_rfp*x(k,1))  

      if (x(k,1)<=0.5_rfp) then 
        kappa(k) = kappa1
      else
        kappa(k) = kappa2
      end if
    end do    
  end subroutine varkappa
end module eqconst

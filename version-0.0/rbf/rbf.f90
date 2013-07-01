! $$$ rbf.f90 $$$
!
module radial_basis_function
use fp
!
implicit none
!
private
!
public :: dphi, query_phi
!
interface dphi
  module procedure dphi_c, dphi_r, dphi_c_i, dphi_r_i, dphi_c_ij, dphi_r_ij
end interface
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!! Written by:  Elisabeth Larsson, Uppsala University, Sweden.
!! Contact:     Elisabeth.Larsson@it.uu.se 
!!
!! Purpose:     Evaluates RBFs, phi(r)=phi(||x-c||), or their derivatives.
!!
!! Subroutines: The generic interface dphi allows three alternative
!!              parameter lists depending on which types of derivatives 
!!              are to be computed.
!! 
!!              call dphi(phi,nprime,nd,epsilon,r,F)
!!              call dphi(phi,nprime,epsilon,r,ri,F)
!!              call dphi(phi,nprime,epsilon,r,ri,rj,F)
!!
!! Input:       phi (character(len=*)) 
!!              'mq'    Multiquadric RBFs     phi=sqrt(1+(ep*r)^2)
!!              'iq'    Inverse quadratics    phi=1/(1+(ep*r)^2)
!!              'imq'   Inverse multiquadrics phi=1/sqrt(1+(ep*r)^2)
!!              'gauss' Gaussians             phi=exp(-(ep*r)^2)
!!              'r*'    * can be any number, but should be odd
!!                      for example r3 => phi=|r|^3
!!              'tps*'  * should be an even number, tps4=r^4*log(r) 
!!
!!              nprime (character/integer/integer(1:2))
!!              '0','L' or 'H' for function value, Laplacian or 
!!              Laplacian squared in first call type
!!   
!!              1, 2, 3 or 4 for first through fourth derivatives with
!!              respect to one of the coordinates. Second call type.
!!
!!              (1,1) or (2,2) for mixed derivatives. Third call type.
!!              Note that d^2 phi/dx dx /= d^2 phi/dx^2.
!!
!!              nd (integer) The number of space dimensions.
!!
!!              epsilon (real/complex) The shape parameter
!!
!!              r (real(1:,1:)) Distance matrix r_ij=||x_i-x_j||
!!
!!              ri (real(1:,1:)) Must be of same size as r.
!!              The signed distance in the coordinate direction in which the 
!!              derivative is taken. ri_ij=x_{i,d}-x_{j,d}
!!
!!              rj (real(1:,1:)) As ri. The signed distance in the second
!!              direction for the mixed derivatives.
!!
!! Output:      F (real(1:,1:)/complex(1:,1:)) Of same size as r, ri, rj.
!!              The RBF function or derivative value. 
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
contains
!
!---------------------------------------
! query_phi returns a flag telling if the RBF has a shape parameter and 
! also returns the number of derivatives that are implemented
!
  subroutine query_phi(phi,with_ep,maxprime)
  character(len=*), intent(in) :: phi
  logical, intent(out) :: with_ep
  integer, intent(out) :: maxprime
!
  integer :: p
    !
    ! maxprime is the highest implemented derivative (0=0,L=2,H=4)
    !
    select case (trim(phi))
    case ('mq','imq','iq','gauss')
      with_ep = .true.
      maxprime = 4
    case ('a10','gs4','mq30','gs30')
      with_ep = .true.
      maxprime = 0
    case default
      if (phi(1:1)=='r') then
        with_ep = .false.
        read(phi(2:),*) p
        maxprime = min(p-1,4)
      else if (phi(1:3)=='tps') then
        with_ep = .false.
        read(phi(4:),*) p
        maxprime = min(p-2,4)
      else
        write(*,*) 'Error: The basis function ',phi,' is not implemented'
        stop
      end if      
    end select
  end subroutine query_phi
!
!---------------------------------------
! dphi_r and dphi_c computes the RBFs or their derivatives for real/complex
! epsilon. If a new RBF is added it must be added to both.
!
  subroutine dphi_c(phi,nprime,nd,epsilon,r,F)
  character(len=*), intent(in) :: phi
  character, intent(in) :: nprime
  integer, intent(in) :: nd
  complex(kind=cfp), intent(in) :: epsilon
  real(kind=rfp), dimension(1:,1:), intent(in) :: r
  complex(kind=cfp), dimension(1:,1:), intent(out) :: F
!
  integer :: p
  real(kind=rfp) :: pr
  complex(kind=cfp) :: ep2,ep4,ep6,ep8
  complex(kind=cfp), dimension(1:size(F,1),1:size(F,2)) :: tmp

    ep2 = epsilon*epsilon
    ep4 = ep2*ep2
    ep6 = ep4*ep2
    ep8 = ep4*ep4
!
    tmp = 1.0_rfp+(epsilon*r)**2.0_rfp
!
    if (trim(phi)=='iq') then
      if (nprime=='0') then
        F = 1.0_rfp/tmp
      else if (nprime=='L') then
        F = -2.0_rfp*nd*ep2*tmp**(-2.0_rfp)
        F = F + 8.0_rfp*ep4*(r**2.0_rfp)*tmp**(-3.0_rfp)
      else if (nprime=='H') then
        F = 8.0_rfp*nd*(nd+2)*ep4*tmp**(-3.0_rfp)
        F = F - 96.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp**(-4.0_rfp)
        F = F + 384.0_rfp*ep8*(r**4.0_rfp)*tmp**(-5.0_rfp)
      end if 
!
    else if (trim(phi)=='mq') then
      tmp = sqrt(tmp)
      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
        F = nd*ep2/tmp 
        F = F - ep4*(r**2.0_rfp)*tmp**(-3.0_rfp)
      else if (nprime=='H') then
        F = -nd*(nd+2)*ep4*tmp**(-3.0_rfp)
        F = F + 6.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp**(-5.0_rfp)
        F = F - 15.0_rfp*ep8*(r**4.0_rfp)*tmp**(-7.0_rfp)
      end if 
!
    else if (trim(phi)=='gauss') then
      tmp = exp(-(epsilon*r)**2.0_rfp)
      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
        F = (-2.0_rfp*nd*ep2 + 4*ep4*(r**2.0_rfp))*tmp 
      else if (nprime=='H') then
        F = 4.0_rfp*nd*(nd+2)*ep4*tmp
        F = F - 16.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp
        F = F + 16.0_rfp*ep8*(r**4.0_rfp)*tmp 
      end if 
!
    else if (trim(phi)=='mq30') then
      tmp = sqrt(tmp)
      if (nprime=='0') then
        F = tmp -1.0_rfp/16.0_rfp*(epsilon*r)**6.0_rfp
      else if (nprime=='L') then
      else if (nprime=='H') then
      end if 
!
    else if (trim(phi)=='gs30') then
      tmp = exp(-(epsilon*r)**2.0_rfp)
      if (nprime=='0') then
        F = tmp +1.0_rfp/6.0_rfp*(epsilon*r)**6.0_rfp
      else if (nprime=='L') then
      else if (nprime=='H') then
      end if 
!
    else if (trim(phi)=='gs4') then
      tmp = exp(-(epsilon*r)**4.0_rfp)
      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
!        F = (-2.0_rfp*nd*ep2 + 4*ep4*(r**2.0_rfp))*tmp 
      else if (nprime=='H') then
!        F = 4.0_rfp*nd*(nd+2)*ep4*tmp
!        F = F - 16.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp
!        F = F + 16.0_rfp*ep8*(r**4.0_rfp)*tmp 
      end if 
!
    else if (trim(phi)=='a10') then
      tmp = exp(-(epsilon*r)**2.0_rfp) + (epsilon*r)**2.0_rfp
      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
!        F = (-2.0_rfp*nd*ep2 + 4*ep4*(r**2.0_rfp))*tmp 
      else if (nprime=='H') then
!        F = 4.0_rfp*nd*(nd+2)*ep4*tmp
!        F = F - 16.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp
!        F = F + 16.0_rfp*ep8*(r**4.0_rfp)*tmp 
      end if 
!
    else if (trim(phi)=='imq') then
      tmp = 1.0_rfp/sqrt(tmp)
      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
        F = (3-nd)*ep2*tmp**3.0_rfp 
        F = F - 3.0_rfp*ep2*tmp**(5.0_rfp)
      else if (nprime=='H') then
        F = 3.0_rfp*ep4*(nd*nd-4*nd+3)*tmp**5.0_rfp
        F = F - 6.0_rfp*(nd-1)*ep4*(-3+2*ep2*r**2.0_rfp)*tmp**7.0_rfp
        F = F + 3.0_rfp*ep4*(3.0_rfp-24.0_rfp*ep2*r**2.0_rfp + &
            8.0_rfp*ep4*r**4.0_rfp)*tmp**9.0_rfp
      end if 
!
    else if (phi(1:1)=='r') then
      read(phi(2:),*) p
!      write(*,*) 'Read degree ',p
      pr = 1.0_rfp*p
      if (nprime=='0') then
        F = r**pr
      else if (nprime=='L') then
        F = (nd+p-2)*p*r**(pr-2.0_rfp)
      else if (nprime=='H') then
        F = ((-nd**2+4*nd-3)*p + &
             ( nd**2-4*nd+3)*p*(p-1) + &
             (       2*nd-2)*p*(p-1)*(p-2) + &
                             p*(p-1)*(p-2)*(p-3))*r**(pr-4.0_rfp)
      end if 
!
    else if (phi(1:3)=='tps') then
      read(phi(4:),*) p
      pr = 1.0_rfp*p
      tmp = 0.0_rfp
      where (r==0.0_rfp)
        tmp=1.0_rfp
      end where
!    
      if (nprime=='0') then
        F = r**pr*log(r+tmp)
      else if (nprime=='L') then
        F = (nd+p-2)*p*r**(pr-2.0_rfp)*log(r+tmp)+(nd+2*p-2)*r**(pr-2.0_rfp)
      else if (nprime=='H') then
        F = p*(p-2)*(nd+p-2)*(nd+p-4)*r**(pr-4.0_rfp)*log(r+tmp)
        F = F + 2*(nd+2*p-4)*(nd*(p-1)+p*p-4*p+2)*r**(pr-4.0_rfp)
      end if 
    end if
!
  end subroutine dphi_c
!
!
  subroutine dphi_r(phi,nprime,nd,epsilon,r,F)
  character(len=*), intent(in) :: phi
  character, intent(in) :: nprime
  integer, intent(in) :: nd
  real(kind=rfp), intent(in) :: epsilon
  real(kind=rfp), dimension(1:,1:), intent(in) :: r
  real(kind=rfp), dimension(1:,1:) :: F
!
  integer :: p
  real(kind=rfp) :: ep2,ep4,ep6,ep8,pr
  real(kind=rfp), dimension(1:size(F,1),1:size(F,2)) :: tmp

    ep2 = epsilon*epsilon
    ep4 = ep2*ep2
    ep6 = ep4*ep2
    ep8 = ep4*ep4

    tmp = 1.0_rfp+(epsilon*r)**2.0_rfp

    if (trim(phi)=='iq') then
      if (nprime=='0') then
        F = 1.0_rfp/tmp
      else if (nprime=='L') then
        F = -2.0_rfp*nd*ep2*tmp**(-2.0_rfp)
        F = F + 8.0_rfp*ep4*(r**2.0_rfp)*tmp**(-3.0_rfp)
      else if (nprime=='H') then
        F = 8.0_rfp*nd*(nd+2)*ep4*tmp**(-3.0_rfp)
        F = F - 96.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp**(-4.0_rfp)
        F = F + 384.0_rfp*ep8*(r**4.0_rfp)*tmp**(-5.0_rfp)
      end if 
!
    else if (trim(phi)=='mq') then
      tmp = sqrt(tmp)
      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
        F = nd*ep2/tmp 
        F = F - ep4*(r**2.0_rfp)*tmp**(-3.0_rfp)
      else if (nprime=='H') then
        F = -nd*(nd+2)*ep4*tmp**(-3.0_rfp)
        F = F + 6.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp**(-5.0_rfp)
        F = F - 15.0_rfp*ep8*(r**4.0_rfp)*tmp**(-7.0_rfp)
      end if 
!
    else if (trim(phi)=='gauss') then
      tmp = exp(-(epsilon*r)**2.0_rfp)

      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
        F = (-2.0_rfp*nd*ep2 + 4*ep4*(r**2.0_rfp))*tmp 
      else if (nprime=='H') then
        F = 4.0_rfp*nd*(nd+2)*ep4*tmp
        F = F - 16.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp
        F = F + 16.0_rfp*ep8*(r**4.0_rfp)*tmp 
      end if 
!
    else if (trim(phi)=='mq30') then
      tmp = sqrt(tmp)
      if (nprime=='0') then
        F = tmp -1.0_rfp/16.0_rfp*(epsilon*r)**6.0_rfp
      else if (nprime=='L') then
      else if (nprime=='H') then
      end if 
!
    else if (trim(phi)=='gs30') then
      tmp = exp(-(epsilon*r)**2.0_rfp)
      if (nprime=='0') then
        F = tmp +1.0_rfp/6.0_rfp*(epsilon*r)**6.0_rfp
      else if (nprime=='L') then
      else if (nprime=='H') then
      end if 
!
    else if (trim(phi)=='gs4') then
      tmp = exp(-(epsilon*r)**4.0_rfp)

      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
!        F = (-2.0_rfp*nd*ep2 + 4*ep4*(r**2.0_rfp))*tmp 
      else if (nprime=='H') then
!        F = 4.0_rfp*nd*(nd+2)*ep4*tmp
!        F = F - 16.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp
!        F = F + 16.0_rfp*ep8*(r**4.0_rfp)*tmp 
      end if 
!
    else if (trim(phi)=='a10') then
      tmp = exp(-(epsilon*r)**2.0_rfp) + (epsilon*r)**2.0_rfp

      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
!        F = (-2.0_rfp*nd*ep2 + 4*ep4*(r**2.0_rfp))*tmp 
      else if (nprime=='H') then
!        F = 4.0_rfp*nd*(nd+2)*ep4*tmp
!        F = F - 16.0_rfp*(nd+2)*ep6*(r**2.0_rfp)*tmp
!        F = F + 16.0_rfp*ep8*(r**4.0_rfp)*tmp 
      end if 
!
    else if (trim(phi)=='imq') then
      tmp = 1.0_rfp/sqrt(tmp)
      if (nprime=='0') then
        F = tmp
      else if (nprime=='L') then
        F = (3-nd)*ep2*tmp**3.0_rfp 
        F = F - 3.0_rfp*ep2*tmp**(5.0_rfp)
      else if (nprime=='H') then
        F = 3.0_rfp*ep4*(nd*nd-4*nd+3)*tmp**5.0_rfp
        F = F - 6.0_rfp*(nd-1)*ep4*(-3+2*ep2*r**2.0_rfp)*tmp**7.0_rfp
        F = F + 3.0_rfp*ep4*(3.0_rfp-24.0_rfp*ep2*r**2.0_rfp + &
             8.0_rfp*ep4*r**4.0_rfp)*tmp**9.0_rfp
      end if 
!
    else if (phi(1:1)=='r') then
      read(phi(2:),*) p
!      write(*,*) 'Read degree ',p
      pr = 1.0_rfp*p
      if (nprime=='0') then
        F = r**pr
      else if (nprime=='L') then
        F = (nd+p-2)*p*r**(pr-2.0_rfp)
      else if (nprime=='H') then
        F = ((-nd**2+4*nd-3)*p + &
             ( nd**2-4*nd+3)*p*(p-1) + &
             (       2*nd-2)*p*(p-1)*(p-2) + &
                             p*(p-1)*(p-2)*(p-3))*r**(pr-4.0_rfp)
      end if 
!
    else if (phi(1:3)=='tps') then
      read(phi(4:),*) p
      pr = 1.0_rfp*p
      tmp = 0.0_rfp
      where (r==0.0_rfp)
        tmp=1.0_rfp
      end where
    
      if (nprime=='0') then
        F = r**pr*log(r+tmp)
      else if (nprime=='L') then
        F = (nd+p-2)*p*r**(pr-2.0_rfp)*log(r+tmp)+(nd+2*p-2)*r**(pr-2.0_rfp)
      else if (nprime=='H') then
        F = p*(p-2)*(nd+p-2)*(nd+p-4)*r**(pr-4.0_rfp)*log(r+tmp)
        F = F + 2*(nd+2*p-4)*(nd*(p-1)+p*p-4*p+2)*r**(pr-4.0_rfp)
      end if 
    end if
!
  end subroutine dphi_r
!
!
  subroutine dphi_c_i(phi,nprime,epsilon,r,ri,F)
  character(len=*), intent(in) :: phi
  integer, intent(in) :: nprime
  complex(kind=cfp), intent(in) :: epsilon
  real(kind=rfp), dimension(1:,1:), intent(in) :: r,ri
  complex(kind=cfp), dimension(1:,1:), intent(out) :: F
!
  integer :: p
  real(kind=rfp) :: pr
  complex(kind=cfp) :: ep2,ep4,ep6,ep8
  complex(kind=cfp), dimension(1:size(F,1),1:size(F,2)) :: tmp

    ep2 = epsilon*epsilon
    ep4 = ep2*ep2
    ep6 = ep4*ep2
    ep8 = ep4*ep4

    tmp = 1.0_rfp+(epsilon*r)**2.0_rfp

    if (trim(phi)=='iq') then
      if (nprime==1) then
        F = -2.0_rfp*ep2*ri*tmp**(-2.0_rfp)
      else if (nprime==2) then
        F = -2.0_rfp*ep2*tmp**(-2.0_rfp)
        F = F + 8.0_rfp*ep4*(ri**2.0_rfp)*tmp**(-3.0_rfp)
      else if (nprime==3) then
        F = -48.0_rfp*ep6*(ri**3.0_rfp)*tmp**(-4.0_rfp)
        F = F + 24.0_rfp*ep4*ri*tmp**(-3.0_rfp)
      else if (nprime==4) then
        F = 384.0_rfp*ep8*(ri**4.0_rfp)*tmp**(-5.0_rfp)
        F = F - 288.0_rfp*ep6*(ri**2.0_rfp)*tmp**(-4.0_rfp)
        F = F + 24.0_rfp*ep4*tmp**(-3.0_rfp)
      end if 
!
    else if (trim(phi)=='mq') then
      tmp = sqrt(tmp)
      if (nprime==1) then
        F = ep2*ri/tmp
      else if (nprime==2) then
        F = ep2*(1.0_rfp+r**2.0_rfp-ri**2.0_rfp)*tmp**(-3.0_rfp)
      else if (nprime==3) then
        F = -3.0_rfp*ep4*ri*(1.0_rfp+r**2.0_rfp-ri**2.0_rfp)*tmp**(-5.0_rfp)
      else if (nprime==4) then
        F = -3.0_rfp*ep4*tmp**(-3.0_rfp)
        F = F + 18.0_rfp*ep6*(ri**2.0_rfp)*tmp**(-5.0_rfp)
        F = F - 15.0_rfp*ep8*(ri**4.0_rfp)*tmp**(-7.0_rfp)
      end if 
!
    else if (trim(phi)=='gauss') then
      tmp = exp(-(epsilon*r)**2.0_rfp)
      if (nprime==1) then
        F = -2.0_rfp*ep2*ri*tmp
      else if (nprime==2) then
        F = (-2.0_rfp*ep2 + 4.0_rfp*ep4*(ri**2.0_rfp))*tmp 
      else if (nprime==3) then
        F = (12*ep4*ri-8*ep6*ri**3.0_rfp)*tmp 
      else if (nprime==4) then
        F = 12.0_rfp*ep4 - 48.0_rfp*ep6*(ri**2.0_rfp)
        F = F + 16.0_rfp*ep8*(ri**4.0_rfp)
        F = F*tmp 
      end if 
!
    else if (trim(phi)=='imq') then
      tmp = 1.0_rfp/sqrt(tmp)
      if (nprime==1) then
        F = -ep2*ri*tmp**3.0_rfp
      else if (nprime==2) then
        F = -ep2*tmp**3.0_rfp 
        F = F + 3.0_rfp*ep4*(ri**2.0_rfp)*tmp**(5.0_rfp)
      else if (nprime==3) then
        F = -15.0_rfp*ep6*(ri**3.0_rfp)*tmp**(7.0_rfp)
        F = F + 9.0_rfp*ep4*ri*tmp**(5.0_rfp)
      else if (nprime==4) then
        F = 9.0_rfp*ep4*tmp**5.0_rfp
        F = F - 90.0_rfp*ep6*(ri**2.0_rfp)*tmp**7.0_rfp
        F = F + 105.0_rfp*ep8*(ri**4.0_rfp)*tmp**9.0_rfp
      end if 
!
    else if (phi(1:1)=='r') then
      ! Tmp is needed to handle 0/0 situations, where the result 
      ! is supposed to be 0 (ri=0 if r=0)
      tmp = 0.0_rfp
      where (r==0.0_rfp) tmp = 1.0_rfp
      tmp = tmp + r
      read(phi(2:),*) p
!      write(*,*) 'Read degree ',p
      pr = 1.0_rfp*p
      if (nprime==1) then
        F = pr*ri*r**(pr-2.0_rfp)
      else if (nprime==2) then
        F = pr*r**(pr-2.0_rfp)
        F = F + p*(p-2)*ri**2.0_rfp*tmp**(pr-4.0_rfp)
      else if (nprime==3) then
        F = 3*p*(p-2)*ri*r**(pr-4.0_rfp)
        F = F + p*(p-2)*(p-4)*ri**3.0_rfp*tmp**(pr-6.0_rfp)
      else if (nprime==4) then
        F = 3*p*(p-2)*r**(pr-4.0_rfp)
        F = F + 6*p*(p-2)*(p-4)*ri**2.0_rfp*tmp**(pr-6.0_rfp)
        F = F + p*(p-2)*(p-4)*(p-6)*ri**4.0_rfp*tmp**(pr-8.0_rfp)
      end if 
!
    else if (phi(1:3)=='tps') then
      read(phi(4:),*) p
      pr = 1.0_rfp*p
      ! Tmp handles cases with 0/0, where the final result is 0
      tmp = 0.0_rfp
      where (r==0.0_rfp) tmp=1.0_rfp
      tmp = tmp + r
!    
      if (nprime==1) then
        F = ri*r**(pr-2.0_rfp)*(pr*log(tmp)+1.0_rfp)
      else if (nprime==2) then
        F = r**(pr-2.0_rfp)*(pr*log(tmp)+1.0_rfp) 
        F = F + ri**2.0_rfp*tmp**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
      else if (nprime==3) then
        F = 3.0_rfp*ri*r**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
        F = F + ri**3.0_rfp*tmp**(pr-6.0_rfp)*(p*(p-2)*(p-4)*log(tmp) + &
                3*p**2-12*p+8)  
      else if (nprime==4) then
        F = 3.0_rfp*r**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
        F = F + 6.0_rfp*ri**2.0_rfp*tmp**(pr-6.0_rfp)* &
                (p*(p-2)*(p-4)*log(tmp) + 3*p**2-12*p+8)
        F = F + ri**4.0_rfp*tmp**(pr-8.0_rfp)*(p*(p-2)*(p-4)*(p-6)*log(tmp)+&
                4*(p-3)*(p*p-6*p+4))
      end if 
    end if
  end subroutine dphi_c_i
!
!
  subroutine dphi_r_i(phi,nprime,epsilon,r,ri,F)
  character(len=*), intent(in) :: phi
  integer, intent(in) :: nprime
  real(kind=rfp), intent(in) :: epsilon
  real(kind=rfp), dimension(1:,1:), intent(in) :: r,ri
  real(kind=rfp), dimension(1:,1:), intent(out) :: F
!
  integer :: p
  real(kind=rfp) :: ep2,ep4,ep6,ep8,pr
  real(kind=rfp), dimension(1:size(F,1),1:size(F,2)) :: tmp

    ep2 = epsilon*epsilon
    ep4 = ep2*ep2
    ep6 = ep4*ep2
    ep8 = ep4*ep4

    tmp = 1.0_rfp+(epsilon*r)**2.0_rfp

    if (trim(phi)=='iq') then
      if (nprime==1) then
        F = -2.0_rfp*ep2*ri*tmp**(-2.0_rfp)
      else if (nprime==2) then
        F = -2.0_rfp*ep2*tmp**(-2.0_rfp)
        F = F + 8.0_rfp*ep4*(ri**2.0_rfp)*tmp**(-3.0_rfp)
      else if (nprime==3) then
        F = -48.0_rfp*ep6*(ri**3.0_rfp)*tmp**(-4.0_rfp)
        F = F + 24.0_rfp*ep4*ri*tmp**(-3.0_rfp)
      else if (nprime==4) then
        F = 384.0_rfp*ep8*(ri**4.0_rfp)*tmp**(-5.0_rfp)
        F = F - 288.0_rfp*ep6*(ri**2.0_rfp)*tmp**(-4.0_rfp)
        F = F + 24.0_rfp*ep4*tmp**(-3.0_rfp)
      end if 
!
    else if (trim(phi)=='mq') then
      tmp = sqrt(tmp)
      if (nprime==1) then
        F = ep2*ri/tmp
      else if (nprime==2) then
        F = ep2*(1.0_rfp+r**2.0_rfp-ri**2.0_rfp)*tmp**(-3.0_rfp)
      else if (nprime==3) then
        F = -3.0_rfp*ep4*ri*(1.0_rfp+r**2.0_rfp-ri**2.0_rfp)*tmp**(-5.0_rfp)
      else if (nprime==4) then
        F = -3.0_rfp*ep4*tmp**(-3.0_rfp)
        F = F + 18.0_rfp*ep6*(ri**2.0_rfp)*tmp**(-5.0_rfp)
        F = F - 15.0_rfp*ep8*(ri**4.0_rfp)*tmp**(-7.0_rfp)
      end if 
!
    else if (trim(phi)=='gauss') then
      tmp = exp(-(epsilon*r)**2.0_rfp)
      if (nprime==1) then
        F = -2.0_rfp*ep2*ri*tmp
      else if (nprime==2) then
        F = (-2.0_rfp*ep2 + 4.0_rfp*ep4*(ri**2.0_rfp))*tmp 
      else if (nprime==3) then
        F = (12*ep4*ri-8*ep6*ri**3.0_rfp)*tmp 
      else if (nprime==4) then
        F = 12.0_rfp*ep4 - 48.0_rfp*ep6*(ri**2.0_rfp)
        F = F + 16.0_rfp*ep8*(ri**4.0_rfp)
        F = F*tmp 
      end if 
!
    else if (trim(phi)=='imq') then
      tmp = 1.0_rfp/sqrt(tmp)
      if (nprime==1) then
        F = -ep2*ri*tmp**3.0_rfp
      else if (nprime==2) then
        F = -ep2*tmp**3.0_rfp 
        F = F + 3.0_rfp*ep4*(ri**2.0_rfp)*tmp**(5.0_rfp)
      else if (nprime==3) then
        F = -15.0_rfp*ep6*(ri**3.0_rfp)*tmp**(7.0_rfp)
        F = F + 9.0_rfp*ep4*ri*tmp**(5.0_rfp)
      else if (nprime==4) then
        F = 9.0_rfp*ep4*tmp**5.0_rfp
        F = F - 90.0_rfp*ep6*(ri**2.0_rfp)*tmp**7.0_rfp
        F = F + 105.0_rfp*ep8*(ri**4.0_rfp)*tmp**9.0_rfp
      end if 
!
    else if (phi(1:1)=='r') then
      ! Tmp is needed to handle 0/0 situations, where the result 
      ! is supposed to be 0 (ri=0 if r=0)
      tmp = 0.0_rfp
      where (r==0.0_rfp) tmp = 1.0_rfp
      tmp = tmp + r
      read(phi(2:),*) p
!      write(*,*) 'Read degree ',p
      pr = 1.0_rfp*p
      if (nprime==1) then
        F = pr*ri*r**(pr-2.0_rfp)
      else if (nprime==2) then
        F = pr*r**(pr-2.0_rfp)
        F = F + p*(p-2)*ri**2.0_rfp*tmp**(pr-4.0_rfp)
      else if (nprime==3) then
        F = 3*p*(p-2)*ri*r**(pr-4.0_rfp)
        F = F + p*(p-2)*(p-4)*ri**3.0_rfp*tmp**(pr-6.0_rfp)
      else if (nprime==4) then
        F = 3*p*(p-2)*r**(pr-4.0_rfp)
        F = F + 6*p*(p-2)*(p-4)*ri**2.0_rfp*tmp**(pr-6.0_rfp)
        F = F + p*(p-2)*(p-4)*(p-6)*ri**4.0_rfp*tmp**(pr-8.0_rfp)
      end if 
!
    else if (phi(1:3)=='tps') then
      read(phi(4:),*) p
      pr = 1.0_rfp*p
      ! Tmp handles cases with 0/0, where the final result is 0
      tmp = 0.0_rfp
      where (r==0.0_rfp) tmp=1.0_rfp
      tmp = tmp + r
    
      if (nprime==1) then
        F = ri*r**(pr-2.0_rfp)*(pr*log(tmp)+1.0_rfp)
      else if (nprime==2) then
        F = r**(pr-2.0_rfp)*(pr*log(tmp)+1.0_rfp) 
        F = F + ri**2.0_rfp*tmp**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
      else if (nprime==3) then
        F = 3.0_rfp*ri*r**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
        F = F + ri**3.0_rfp*tmp**(pr-6.0_rfp)*(p*(p-2)*(p-4)*log(tmp) + &
                3*p**2-12*p+8)  
      else if (nprime==4) then
        F = 3.0_rfp*r**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
        F = F + 6.0_rfp*ri**2.0_rfp*tmp**(pr-6.0_rfp)* &
                (p*(p-2)*(p-4)*log(tmp) + 3*p**2-12*p+8)
        F = F + ri**4.0_rfp*tmp**(pr-8.0_rfp)*(p*(p-2)*(p-4)*(p-6)*log(tmp)+&
                4*(p-3)*(p*p-6*p+4))
      end if 
    end if
  end subroutine dphi_r_i
!
  subroutine dphi_c_ij(phi,nprime,epsilon,r,ri,rj,F)
  character(len=*), intent(in) :: phi
  integer, dimension(1:2), intent(in) :: nprime
  complex(kind=cfp), intent(in) :: epsilon
  real(kind=rfp), dimension(1:,1:), intent(in) :: r,ri,rj
  complex(kind=cfp), dimension(1:,1:), intent(out) :: F
!
  integer :: p
  real(kind=rfp) :: pr
  complex(kind=cfp) :: ep2,ep4,ep6,ep8
  complex(kind=cfp), dimension(1:size(F,1),1:size(F,2)) :: tmp

    if (minval(nprime)<1 .or. maxval(nprime)>2 .or. &
         nprime(1)/=nprime(2)) then        
      write(*,*) 'Error: The given combination of nprime is not ', &
             'implemented in rbf.f90'
      stop
    end if

    ep2 = epsilon*epsilon
    ep4 = ep2*ep2
    ep6 = ep4*ep2
    ep8 = ep4*ep4

    tmp = 1.0_rfp+(epsilon*r)**2.0_rfp

    if (trim(phi)=='iq') then
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = 8.0_rfp*ep4*ri*rj*tmp**(-3.0_rfp)
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = 8.0_rfp*ep4*tmp**(-3.0_rfp)
        F = F - 48.0_rfp*ep6*(ri*ri+rj*rj)*tmp**(-4.0_rfp)
        F = F + 384.0_rfp*ep8*ri*ri*rj*rj*tmp**(-5.0_rfp)
      end if 
!
    else if (trim(phi)=='mq') then
      tmp = sqrt(tmp)
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = -ep4*ri*rj*tmp**(-3.0_rfp)
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = -ep4*tmp**(-3.0_rfp)
        F = F + 3.0_rfp*ep6*(ri*ri+rj*rj)*tmp**(-5.0_rfp)
        F = F - 15.0_rfp*ep8*ri*ri*rj*rj*tmp**(-7.0_rfp)
      end if 
!
    else if (trim(phi)=='gauss') then
      tmp = exp(-(epsilon*r)**2.0_rfp)
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = 4.0_rfp*ep4*ri*rj*tmp
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = 4.0_rfp*ep4-8.0_rfp*ep6*(ri*ri+rj*rj)+16.0_rfp*ep8*ri*ri*rj*rj
        F = F*tmp
      end if 
!
    else if (trim(phi)=='imq') then
      tmp = 1.0_rfp/sqrt(tmp)
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = 3.0_rfp*ri*rj*ep4*tmp**5.0_rfp
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = 3.0_rfp*ep4*tmp**5.0_rfp
        F = F - 15.0_rfp*(ri*ri+rj*rj)*ep6*tmp**7.0_rfp
        F = F + 105.0_rfp*ri*ri*rj*rj*tmp**9.0_rfp
      end if 
!
    else if (phi(1:1)=='r') then
      ! Tmp is needed to handle 0/0 situations, where the result 
      ! is supposed to be 0 (ri=0 if r=0)
      tmp = 0.0_rfp
      where (r==0.0_rfp) tmp = 1.0_rfp
      tmp = tmp + r
      read(phi(2:),*) p
!      write(*,*) 'Read degree ',p
      pr = 1.0_rfp*p
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = p*(p-2)*ri*rj*tmp**(pr-4.0_rfp)
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = p*(p-2)*tmp**(pr-4.0_rfp)
        F = F + p*(p-2)*(p-4)*(ri*ri+rj*rj)*tmp**(pr-6.0_rfp)
        F = F + p*(p-2)*(p-4)*(p-6)*ri*ri*rj*rj*tmp**(pr-8.0_rfp)
      end if 
!
    else if (phi(1:3)=='tps') then
      read(phi(4:),*) p
      pr = 1.0_rfp*p
      ! Tmp handles cases with 0/0, where the final result is 0
      tmp = 0.0_rfp
      where (r==0.0_rfp) tmp=1.0_rfp
      tmp = tmp + r
!    
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = ri*rj*r**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = r**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
        F = F + (ri*ri+rj*rj)*r**(pr-6.0_rfp)* (p*(p-2)*(p-4)*log(tmp)+ &
                3*p*p-12*p+8)
        F = F + (ri*rj)**2.0_rfp*r**(pr-8.0_rfp)* &
                (p*(p-2)*(p-4)*(p-6)*log(tmp)+4*(p-3)*(p*p-6*p+4))
      end if 
    end if
  end subroutine dphi_c_ij
!
!
  subroutine dphi_r_ij(phi,nprime,epsilon,r,ri,rj,F)
  character(len=*), intent(in) :: phi
  integer, dimension(1:2), intent(in) :: nprime
  real(kind=rfp), intent(in) :: epsilon
  real(kind=rfp), dimension(1:,1:), intent(in) :: r,ri,rj
  real(kind=rfp), dimension(1:,1:), intent(out) :: F
!
  integer :: p
  real(kind=rfp) :: ep2,ep4,ep6,ep8,pr
  real(kind=rfp), dimension(1:size(F,1),1:size(F,2)) :: tmp

    if (minval(nprime)<1 .or. maxval(nprime)>2 .or. &
         nprime(1)/=nprime(2)) then        
      write(*,*) 'Error: The given combination of nprime is not ', &
             'implemented in rbf.f90'
      stop
    end if

    ep2 = epsilon*epsilon
    ep4 = ep2*ep2
    ep6 = ep4*ep2
    ep8 = ep4*ep4

    tmp = 1.0_rfp+(epsilon*r)**2.0_rfp

    if (trim(phi)=='iq') then
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = 8.0_rfp*ep4*ri*rj*tmp**(-3.0_rfp)
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = 8.0_rfp*ep4*tmp**(-3.0_rfp)
        F = F - 48.0_rfp*ep6*(ri*ri+rj*rj)*tmp**(-4.0_rfp)
        F = F + 384.0_rfp*ep8*ri*ri*rj*rj*tmp**(-5.0_rfp)
      end if 
!
    else if (trim(phi)=='mq') then
      tmp = sqrt(tmp)
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = -ep4*ri*rj*tmp**(-3.0_rfp)
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = -ep4*tmp**(-3.0_rfp)
        F = F + 3.0_rfp*ep6*(ri*ri+rj*rj)*tmp**(-5.0_rfp)
        F = F - 15.0_rfp*ep8*ri*ri*rj*rj*tmp**(-7.0_rfp)
      end if 
!
    else if (trim(phi)=='gauss') then
      tmp = exp(-(epsilon*r)**2.0_rfp)
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = 4.0_rfp*ep4*ri*rj*tmp
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = 4.0_rfp*ep4-8.0_rfp*ep6*(ri*ri+rj*rj)+16.0_rfp*ep8*ri*ri*rj*rj
        F = F*tmp
      end if 
!
    else if (trim(phi)=='imq') then
      tmp = 1.0_rfp/sqrt(tmp)
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = 3.0_rfp*ri*rj*ep4*tmp**5.0_rfp
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = 3.0_rfp*ep4*tmp**5.0_rfp
        F = F - 15.0_rfp*(ri*ri+rj*rj)*ep6*tmp**7.0_rfp
        F = F + 105.0_rfp*ri*ri*rj*rj*tmp**9.0_rfp
      end if 
!
    else if (phi(1:1)=='r') then
      ! Tmp is needed to handle 0/0 situations, where the result 
      ! is supposed to be 0 (ri=0 if r=0)
      tmp = 0.0_rfp
      where (r==0.0_rfp) tmp = 1.0_rfp
      tmp = tmp + r
      read(phi(2:),*) p
!      write(*,*) 'Read degree ',p
      pr = 1.0_rfp*p
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = p*(p-2)*ri*rj*tmp**(pr-4.0_rfp)
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = p*(p-2)*tmp**(pr-4.0_rfp)
        F = F + p*(p-2)*(p-4)*(ri*ri+rj*rj)*tmp**(pr-6.0_rfp)
        F = F + p*(p-2)*(p-4)*(p-6)*ri*ri*rj*rj*tmp**(pr-8.0_rfp)
      end if 
!
    else if (phi(1:3)=='tps') then
      read(phi(4:),*) p
      pr = 1.0_rfp*p
      ! Tmp handles cases with 0/0, where the final result is 0
      tmp = 0.0_rfp
      where (r==0.0_rfp) tmp=1.0_rfp
      tmp = tmp + r
!    
      if (nprime(1)==1 .and. nprime(2)==1) then
        F = ri*rj*r**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
      else if (nprime(1)==2 .and. nprime(2)==2) then
        F = r**(pr-4.0_rfp)*(p*(p-2)*log(tmp)+2*p-2)
        F = F + (ri*ri+rj*rj)*r**(pr-6.0_rfp)* (p*(p-2)*(p-4)*log(tmp)+ &
                3*p*p-12*p+8)
        F = F + (ri*rj)**2.0_rfp*r**(pr-8.0_rfp)* &
                (p*(p-2)*(p-4)*(p-6)*log(tmp)+4*(p-3)*(p*p-6*p+4))
      end if 
    end if
  end subroutine dphi_r_ij
!
end module radial_basis_function








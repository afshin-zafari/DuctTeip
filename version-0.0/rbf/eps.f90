! $$$ eps.f90 $$$
!
module class_epsilon
use fp
use const
use output
!
implicit none
!
private
!
public :: epsilon
public :: new_eps, new_eps_contour, destruct_eps, query_eps, &
          get_eps, get_eps_r, get_eps_v, write_eps, minabs_eps
!
type epsilon
  private
  real(kind=rfp), dimension(:), pointer :: x,y,r,t
end type epsilon
!
interface new_eps
  module procedure new_eps_cart,new_eps_pol
end interface
!
interface get_eps_v
  module procedure get_eps_v_r, get_eps_v_c
end interface
!
contains
!
  function new_eps_cart(logep,rgx,Nx,rgy,Ny) result(ep)
  logical, intent(in) :: logep
  integer, optional :: Nx,Ny
  real(kind=rfp), dimension(1:2), optional :: rgx,rgy
  !
  type(epsilon) :: ep

    if (present(rgx) .and. present(Nx)) then
      if (Nx > 0 .and. rgx(2)>=rgx(1) .and. &
           .not.(rgx(1)<=0.0_rfp .and. logep) ) then
        allocate(ep%x(1:Nx))
        if (logep) then
          call fill_log(rgx,Nx,ep%x)
        else
          call fill_lin(rgx,Nx,ep%x)
        end if
      else
        write(*,*) 'Warning: Incorrect data given for x in new_eps'
      end if
    else
      nullify(ep%x)
    end if
!
    if (present(rgy) .and. present(Ny)) then
      if (Ny > 0 .and. rgy(2)>=rgy(1) .and. &
           .not.(rgy(1)<=0.0_rfp .and. logep) ) then
        allocate(ep%y(1:Ny))
        if (logep) then
          call fill_log(rgy,Ny,ep%y)
        else
          call fill_lin(rgy,Ny,ep%y)
        end if
      else
        write(*,*) 'Warning: Incorrect data given for y in new_eps'
      end if
    else
      nullify(ep%y)
    end if
    nullify(ep%r,ep%t)
  end function new_eps_cart
!
!
  function new_eps_pol(rgr,Nr,rgt,Nt) result(ep)
  real(kind=rfp), dimension(1:2), intent(in) :: rgr, rgt
  integer :: Nr,Nt 
  !
  type(epsilon) :: ep

    if (Nr > 0 .and. rgr(1) >= 0.0_rfp .and. rgr(2)>=rgr(1)) then
      allocate(ep%r(1:Nr))
      call fill_lin(rgr,Nr,ep%r)
    else
      write(*,*) 'Warning: Incorrect data given for r in new_eps'
    end if
!
    if (Nt > 0 .and. rgt(2)>=rgt(1) .and. rgt(2)-rgt(1)<2*pi) then
      allocate(ep%t(1:Nt))
      call fill_lin(rgt,Nt,ep%t)
    else
      write(*,*) 'Warning: Incorrect data given for theta in new_eps'
    end if
    nullify(ep%x,ep%y)
  end function new_eps_pol
!
!
  function new_eps_contour(r,N,Ntot) result(ep)
  real(kind=rfp), intent(in) :: r
  integer, intent(in) :: N,Ntot
! 
  type(epsilon) :: ep 
!
  integer :: k
  real(kind=rfp) :: h 
!
    if (Ntot>0 .and. N<=Ntot) then
      allocate(ep%t(1:N),ep%r(1:1))
      ep%r(1) = r
      h = 2*pi/real(Ntot,rfp)
      do k=0,N-1 
        ep%t(k+1) = h*k
      end do
    else
      write(*,*) 'Warning: Incorrect data given for new_eps_contour'
    end if
    nullify(ep%x,ep%y)
  end function new_eps_contour
!
!
  subroutine query_eps(ep,re,dims,sz)
  type(epsilon),intent(in) :: ep
  logical, intent(out) :: re
  integer, intent(out) :: dims
  integer, dimension(1:2), intent(out) :: sz
!
    if (associated(ep%y).or.associated(ep%t)) then
      re = .false.
    else if (associated(ep%x)) then
      re = .true.
    else
      write(*,*) 'Warning: Query_ep failed because ep was not allocated'
    end if
    sz = 0
    if (associated(ep%x)) then
      sz(1) = ubound(ep%x,1) 
      if (associated(ep%y)) then
        sz(2) = sz(1)
        sz(1) = ubound(ep%y,1)
        dims = 2
      else
        dims = 1
      end if
    else if (associated(ep%y)) then
      sz(1) = ubound(ep%y,1) 
      dims = 1
    else if (associated(ep%t)) then 
      sz(1) = ubound(ep%t,1)
      if (associated(ep%r)) then
        sz(2) = ubound(ep%r,1)
        dims = 2
      else 
        dims = 1
      end if
    end if 
  end subroutine query_eps

  function get_eps(ep,j,k) result(val)
  type(epsilon), intent(in) :: ep
  integer, intent(in) :: j
  integer, optional :: k
  complex(kind=cfp) :: val
!
  logical :: re
  integer :: dims
  integer, dimension(1:2) :: sz

    call query_eps(ep,re,dims,sz)   
    if (present(k) .and. dims==2) then
      if ( 1<=j .and. j<= sz(1) .and. 1<=k .and. k<=sz(2)) then
        if (associated(ep%x)) then
          val = ep%x(k) + im*ep%y(j) 
        else  
          val = ep%r(k)*exp(im*ep%t(j))
        end if
      else
        write(*,*) 'Error: Index out of range in get_eps'
      end if 
    else if (.not.present(k) .and. dims==1) then
      if ( 1<=j .and. j<= sz(1)) then
        if (associated(ep%x)) then
          val = ep%x(j)
        else if (associated(ep%y)) then
          val = im*ep%y(j)
        else
          val = exp(im*ep%t(j))
        end if
      else
        write(*,*) 'Error: Index out of range in get_eps'
      end if
    end if
  end function get_eps
!
!
  function get_eps_r(ep,j) result(val)
  type(epsilon), intent(in) :: ep
  integer, intent(in) :: j
  real(kind=rfp) :: val
!
  logical :: re
  integer :: dims
  integer, dimension(1:2) :: sz

    call query_eps(ep,re,dims,sz) 
    if (re .and. dims==1) then
      if (1<=j .and. j<=ubound(ep%x,1)) then
        val = ep%x(j)
      else
        write(*,*) 'Error: Index out of range in get_eps_r'
      end if
    else
      write(*,*) 'Error: ep is not real in get_eps_r'
    end if  
  end function get_eps_r
!
!
  subroutine get_eps_v_r(ep,r)
  type(epsilon), intent(in) :: ep
  real(kind=rfp), dimension(1:), intent(out) :: r
!
    if (associated(ep%x)) then
      if (ubound(r,1) == ubound(ep%x,1)) then 
        r = ep%x
      else
        write(*,*) 'Error: The sizes mismatch in get_eps_v'
      end if
    else
      write(*,*) 'Error: ep is not real in get_eps_v'
    end if
  end subroutine get_eps_v_r
!
!
  subroutine get_eps_v_c(ep,c)
  type(epsilon), intent(in) :: ep
  complex(kind=cfp), dimension(1:), intent(out) :: c
!
    if (associated(ep%t)) then
      if (ubound(c,1) == ubound(ep%t,1)) then 
        c = exp(im*ep%t)
      else
        write(*,*) 'Error: The sizes mismatch in get_eps_v'
      end if
    else
      write(*,*) 'Error: ep is not complex in get_eps_v'
    end if
  end subroutine get_eps_v_c
!
!
  subroutine destruct_eps(ep)
  type(epsilon) :: ep
    if (associated(ep%x)) deallocate(ep%x)
    if (associated(ep%y)) deallocate(ep%y)
    if (associated(ep%r)) deallocate(ep%r)
    if (associated(ep%t)) deallocate(ep%t)
  end subroutine destruct_eps
!
!
  subroutine fill_log(rg,N,ep)
  real(kind=rfp), dimension(1:2) :: rg
  integer :: N
  real(kind=rfp), dimension(:) :: ep
  !
  integer :: k
  real(kind=rfp) :: h
  real(kind=rfp), dimension(1:2) :: lrg
  !
    lrg = log10(rg)
    if (N>1) then
      h = (lrg(2)-lrg(1))/real(N-1,rfp)       
    else
      h = 0.0_rfp
    end if
    do k=1,N
      ep(k) = 10.0_rfp**(lrg(1)+h*(k-1))
    end do
  end subroutine fill_log
!
  subroutine fill_lin(rg,N,ep)
  real(kind=rfp), dimension(1:2) :: rg
  integer :: N
  real(kind=rfp), dimension(:) :: ep
  !
  integer :: k
  real(kind=rfp) :: h
  !
    if (N>1) then
      h = (rg(2)-rg(1))/real(N-1,rfp)
    else
      h = 0.0_rfp
    end if
    do k=1,N
      ep(k) = rg(1) + h*(k-1)
    end do
  end subroutine fill_lin
!
!
  function minabs_eps(ep) result(val)
  type(epsilon), intent(in) :: ep
  real(kind=rfp) :: val
!
  logical :: re
  integer :: dims
  integer, dimension(1:2) :: sz

    call query_eps(ep,re,dims,sz)   
    if (dims==2) then
      if (associated(ep%x)) then
        val = sqrt(minval(ep%x)**2 + minval(ep%y)**2) 
      else  
        val = minval(ep%r)
      end if
    else if (dims==1) then
      if (associated(ep%x)) then
        val = minval(ep%x)
      else if (associated(ep%y)) then
        val = minval(ep%y)
      else
        val = 1.0_rfp 
      end if
    end if
  end function minabs_eps
!
!
  subroutine write_eps(ep,fname)
  type(epsilon), intent(in) :: ep
  character(len=*) :: fname
!
  logical :: re
  character(len=80) :: fname2
  integer :: dims,n
  integer, dimension(1:2) :: sz
  complex(kind=cfp), dimension(:), allocatable :: epc
!
    call query_eps(ep,re,dims,sz) 
    if (dims == 1 .and. re ) then
      call write_to_file(fname,ep%x)
    else if (dims==1) then
      allocate(epc(1:sz(1)))
      epc = cmplx(0.0_rfp,ep%y)
      call write_to_file(fname,epc)
      deallocate(epc)
    else if (associated(ep%x)) then
      write(*,*) 'Writing 2D-epsilon data as two files'
      n = len_trim(fname)
      fname2 = fname(1:n-4)//'_x.dat'
      call write_to_file(fname2,ep%x)
      fname2 = fname(1:n-4)//'_y.dat'
      call write_to_file(fname2,ep%y)
    else
      write(*,*) 'Writing 2D-epsilon data as two files'
      n = len_trim(fname)
      fname2 = fname(1:n-4)//'_r.dat'
      call write_to_file(fname2,ep%r)
      fname2 = fname(1:n-4)//'_t.dat'
      call write_to_file(fname2,ep%t)
    end if
  end subroutine write_eps
end module class_epsilon

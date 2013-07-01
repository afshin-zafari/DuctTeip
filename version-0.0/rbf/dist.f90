! $$$ dist.f90 $$$
!
module class_distance
use fp
use class_field
use class_point_set
!
implicit none
!
private
!
public :: distance
public :: new_dist, destruct_dist, get_dist_ptr, dims_dist
!
type distance
  private
  type(field) :: r
  type(field), dimension(:), pointer :: ri
end type distance
!
interface get_dist_ptr
  module procedure get_dist_ptr1,get_dist_ptr2
end interface
!
contains
!
  function new_dist(x,c) result(dist)
  type(point_set) :: x,c
  type(distance) :: dist
!
  logical :: reel 
  integer :: nx,nc,nd,d,k
  real(kind=rfp), dimension(:,:), pointer :: xp,cp 
!
    xp => get_pts_ptr(x)
    cp => get_pts_ptr(c)
    nx = ubound(xp,1)
    nc = ubound(cp,1)
    nd = ubound(xp,2)
    reel = .true.
!
    dist%r = new_field(reel,nx,nc)
    allocate(dist%ri(1:nd))
    do d=1,nd
      dist%ri(d) = new_field(reel,nx,nc)
    end do
! 
! 
    dist%r%r%r2 = 0.0_rfp
    do d=1,nd
      do k=1,nc
        dist%ri(d)%r%r2(:,k) = xp(:,d)-cp(k,d)
      end do 
      dist%r%r%r2 = dist%r%r%r2 + dist%ri(d)%r%r2**2.0_rfp
    end do
    dist%r%r%r2 = sqrt(dist%r%r%r2)
!
  end function new_dist
!
!
  subroutine destruct_dist(dist)
  type(distance) :: dist
!
  integer :: nd,d
!
    nd = ubound(dist%ri,1)
    do d=1,nd
      call destruct_field(dist%ri(d))
    end do
    call destruct_field(dist%r)
!
  end subroutine destruct_dist
!
!
  function get_dist_ptr1(rg_r,rg_c,dist,dim) result(p)
  integer, dimension(1:2), intent(in) :: rg_r,rg_c
  type(distance), intent(in) :: dist
  integer, optional :: dim
!  
  real(kind=rfp), dimension(:,:), pointer :: p
!
    if (present(dim)) then
      p => dist%ri(dim)%r%r2(rg_r(1):rg_r(2),rg_c(1):rg_c(2))
    else
      p => dist%r%r%r2(rg_r(1):rg_r(2),rg_c(1):rg_c(2))
    end if 
  end function get_dist_ptr1
!
!
  function get_dist_ptr2(dist,dim) result(p)
  type(distance), intent(in) :: dist
  integer, optional :: dim
!  
  real(kind=rfp), dimension(:,:), pointer :: p
!
    if (present(dim)) then
      p => dist%ri(dim)%r%r2
    else
      p => dist%r%r%r2
    end if 
  end function get_dist_ptr2
!
!
  function dims_dist(dist) result(nd)
  type(distance), intent(in) :: dist
  integer :: nd
!
    nd = ubound(dist%ri,1)
  end function dims_dist
!
end module class_distance

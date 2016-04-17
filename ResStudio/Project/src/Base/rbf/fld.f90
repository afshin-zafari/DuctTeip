! $$$ fld.f90 $$$
!
module class_field
use fp
!
implicit none
!
private
!
public :: field,ifield,rfield,cfield
public :: new_field,destruct_field,switch_rc_field, copy_field
!
type ifield
integer, dimension(:), pointer :: i1
integer, dimension(:,:), pointer :: i2
integer, dimension(:,:,:), pointer :: i3
end type ifield
!
type rfield
  real(kind=rfp), dimension(:), pointer :: r1
  real(kind=rfp), dimension(:,:), pointer :: r2
  real(kind=rfp), dimension(:,:,:), pointer :: r3
end type rfield
!
type cfield
  complex(kind=cfp), dimension(:), pointer :: c1
  complex(kind=cfp), dimension(:,:), pointer :: c2
  complex(kind=cfp), dimension(:,:,:), pointer :: c3
end type cfield
!
type field
  integer :: irc
  type(ifield) :: i
  type(rfield) :: r
  type(cfield) :: c 
end type field
!
interface new_field
  module procedure new_field_rc,new_field_i,new_field_,new_field0
end interface
!
contains
  function new_ifield(m1,m2,m3) result(f)
  integer, intent(in) :: m1
  integer, optional :: m2,m3
  type(ifield) :: f
!
    if (present(m3) .and. present(m2)) then
       Allocate(f%i3(1:m1,1:m2,1:m3))
       Nullify(f%i2,f%i1)
    Elseif (Present(m2)) Then
       Allocate(f%i2(1:m1,1:m2))
       Nullify(f%i1,f%i3)
    Else
       Allocate(f%i1(1:m1))
       Nullify(f%i2,f%i3)
    end if
  end function new_ifield
!
!
  function new_ifield_() result(f)
  type(ifield) :: f
!
    nullify(f%i1,f%i2,f%i3)
  end function new_ifield_
!
!
  function new_rfield(m1,m2,m3) result(f)
  integer, intent(in) :: m1
  integer, optional :: m2,m3
  type(rfield) :: f
!
    if (present(m3) .and. present(m2)) then
      allocate(f%r3(1:m1,1:m2,1:m3))
      nullify(f%r2,f%r1)
    elseif (present(m2)) then
      allocate(f%r2(1:m1,1:m2))
      nullify(f%r1,f%r3)
    else
      allocate(f%r1(1:m1))
      nullify(f%r2,f%r3)
    end if
  end function new_rfield
!
!
  function new_rfield_() result(f)
  type(rfield) :: f
!
    nullify(f%r1,f%r2,f%r3)
  end function new_rfield_
!
!
  function new_cfield(m1,m2,m3) result(f)
  integer, intent(in) :: m1
  integer, optional :: m2,m3
  type(cfield) :: f
!
    if (present(m3) .and. present(m2)) then
      allocate(f%c3(1:m1,1:m2,1:m3))
      nullify(f%c2,f%c1)
    elseif (present(m2)) then
      allocate(f%c2(1:m1,1:m2))
      nullify(f%c1,f%c3)
    else
      allocate(f%c1(1:m1))
      nullify(f%c2,f%c3)
    end if
  end function new_cfield
!
!
  function new_cfield_() result(f)
  type(cfield) :: f
!
    nullify(f%c1,f%c2,f%c3)
  end function new_cfield_
!
!
  function new_field_rc(real,m1,m2,m3) result(f)
  logical, intent(in) :: real
  integer, intent(in) :: m1
  integer, optional :: m2,m3
  type(field) :: f
! 
    f%i = new_ifield_()
    if (real) then
      f%irc = 1
      f%r = new_rfield(m1,m2,m3)
      f%c = new_cfield_()
    else
      f%irc = 2
      f%c = new_cfield(m1,m2,m3)
      f%r = new_rfield_()
    end if
  end function new_field_rc
!
!
  function new_field_i(m1,m2,m3) result(f)
  integer, intent(in) :: m1
  integer, optional :: m2,m3
  type(field) :: f
!
    f%irc = 0
    f%i = new_ifield(m1,m2,m3)
    f%r = new_rfield_()
    f%c = new_cfield_()
  end function new_field_i
!
!
  function new_field_(g) result(f)
  type(field), intent(in) :: g
  type(field) :: f
  !
  ! f will have the same type and size as g
  !
    f%irc = g%irc
    if (f%irc==0) then
      if (associated(g%i%i1)) then
        f%i = new_ifield(ubound(g%i%i1,1))
      elseif (associated(g%i%i2)) then
        f%i = new_ifield(ubound(g%i%i2,1),ubound(g%i%i2,2))
      elseif (associated(g%i%i3)) then
        f%i = new_ifield(ubound(g%i%i3,1),ubound(g%i%i3,2),ubound(g%i%i3,3))
      end if
      f%r = new_rfield_()
      f%c = new_cfield_()
    elseif (f%irc==1) then
      if (associated(g%r%r1)) then
        f%r = new_rfield(ubound(g%r%r1,1))
      elseif (associated(g%r%r2)) then
        f%r = new_rfield(ubound(g%r%r2,1),ubound(g%r%r2,2))
      elseif (associated(g%r%r3)) then
        f%r = new_rfield(ubound(g%r%r3,1),ubound(g%r%r3,2),ubound(g%r%r3,3))
      end if
      f%i = new_ifield_()
      f%c = new_cfield_()
    elseif (f%irc==2) then
      if (associated(g%c%c1)) then
        f%c = new_cfield(ubound(g%c%c1,1))
      elseif (associated(g%c%c2)) then
        f%c = new_cfield(ubound(g%c%c2,1),ubound(g%c%c2,2))
      elseif (associated(g%c%c3)) then
        f%c = new_cfield(ubound(g%c%c3,1),ubound(g%c%c3,2),ubound(g%c%c3,3))
      end if
      f%r = new_rfield_()
      f%i = new_ifield_()
    else
      f%i = new_ifield_()
      f%r = new_rfield_()
      f%c = new_cfield_()
    end if
  end function new_field_
!
!
  function new_field0() result(f)
  type(field) :: f
  !
    f%irc = 0
    f%i = new_ifield_()
    f%r = new_rfield_()
    f%c = new_cfield_()
  end function new_field0
!
!
  subroutine destruct_field(f)
  type(field) :: f
!
    if (f%irc==1) then
      if (associated(f%r%r3)) then
        deallocate(f%r%r3)
      elseif (associated(f%r%r2)) then
        deallocate(f%r%r2)
      elseif (associated(f%r%r1)) then
        deallocate(f%r%r1)
      end if
    elseif (f%irc==2) then
      if (associated(f%c%c3)) then
        deallocate(f%c%c3)
      elseif (associated(f%c%c2)) then
        deallocate(f%c%c2)
      elseif (associated(f%c%c1)) then
        deallocate(f%c%c1)
      end if
    elseif (f%irc==0) then
      if (associated(f%i%i3)) then
        deallocate(f%i%i3)
      elseif (associated(f%i%i2)) then
        deallocate(f%i%i2)
      elseif (associated(f%i%i1)) then
        deallocate(f%i%i1)
      end if
    end if  
  end subroutine destruct_field
!
! 
  subroutine switch_rc_field(f)
  type(field) :: f
  !
  ! Change from real to complex or vice versa. No data movement.
  !
    if (f%irc==1) then
      if (associated(f%r%r1)) then
        f%c = new_cfield(ubound(f%r%r1,1))
        deallocate(f%r%r1)
      elseif (associated(f%r%r2)) then
        f%c = new_cfield(ubound(f%r%r2,1),ubound(f%r%r2,2))
        deallocate(f%r%r2)
      elseif (associated(f%r%r3)) then
        f%c = new_cfield(ubound(f%r%r3,1),ubound(f%r%r3,2),ubound(f%r%r3,3))
        deallocate(f%r%r3)
      end if
      f%irc = 2
    elseif (f%irc==2) then
      if (associated(f%c%c1)) then
        f%r = new_rfield(ubound(f%c%c1,1))
        deallocate(f%c%c1)
      elseif (associated(f%c%c2)) then
        f%r = new_rfield(ubound(f%c%c2,1),ubound(f%c%c2,2))
        deallocate(f%c%c2)
      elseif (associated(f%c%c3)) then
        f%r = new_rfield(ubound(f%c%c3,1),ubound(f%c%c3,2),ubound(f%c%c3,3))
        deallocate(f%c%c3)
      end if
      f%irc = 1
    end if
  end subroutine switch_rc_field
!
!
  subroutine copy_field(f,g)
  type(field), intent(in) :: f
  type(field), intent(out) :: g
!
    if (f%irc==0) then
      if (associated(f%i%i1)) then
        g%i%i1 = f%i%i1
      elseif (associated(f%i%i2)) then
        g%i%i2 = f%i%i2
      elseif (associated(f%i%i3)) then
        g%i%i3 = f%i%i3
      end if
    elseif (f%irc==1) then
      if (associated(f%r%r1)) then
        g%r%r1 = f%r%r1
      elseif (associated(f%r%r2)) then
        g%r%r2 = f%r%r2
      elseif (associated(f%r%r3)) then
        g%r%r3 = f%r%r3
      end if
    elseif (f%irc==2) then
      if (associated(f%c%c1)) then
        g%c%c1 = f%c%c1
      elseif (associated(g%c%c2)) then
        g%c%c2 = f%c%c2
      elseif (associated(g%c%c3)) then
        g%c%c3 = f%c%c3
      end if
    end if
  end subroutine copy_field
!
!
end module class_field



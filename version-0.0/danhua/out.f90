! $$$ out.f90 $$$

module output
use fp
implicit none
!
private
!
public :: write_to_file 
!
interface write_to_file
  module procedure wtf1_r,wtf1_c,wtf2_r,wtf2_c,wtf3_r,wtf3_c,wtf4_r,wtf4_c
end interface
!
contains
!
  subroutine wtf1_r(fname,data)
  character(len=*), intent(in) :: fname
  real(kind=rfp), dimension(1:) :: data
!
  integer :: k
    open(38,file=fname)
    write(38,*) 1, 0     ! The number of dimensions
    write(38,*) ubound(data,1), 0
    do k=1,ubound(data,1)
      write(38,*) data(k), 0
    end do
    close(38)
  end subroutine wtf1_r
!
!
  subroutine wtf2_r(fname,data)
  character(len=*), intent(in) :: fname
  real(kind=rfp), dimension(1:,1:) :: data
!
  integer :: j,k
    open(38,file=fname)
    write(38,*) 2, 0     ! The number of dimensions
    do k=1,2
      write(38,*) ubound(data,k), 0            ! Size of each dimension
    end do
    do k=1,ubound(data,2)
      do j=1,ubound(data,1)
        write(38,*) data(j,k), 0
      end do
    end do
    close(38)
  end subroutine wtf2_r
!
!
  subroutine wtf3_r(fname,data)
  character(len=*), intent(in) :: fname
  real(kind=rfp), dimension(1:,1:,1:) :: data
!
  integer :: i,j,k
    open(38,file=fname)
    write(38,*) 3, 0     ! The number of dimensions
    do k=1,3
      write(38,*) ubound(data,k), 0
    end do
    do k=1,ubound(data,3)
      do j=1,ubound(data,2)
        do i=1,ubound(data,1)
          write(38,*) data(i,j,k), 0
        end do
      end do
    end do
    close(38)
  end subroutine wtf3_r
!
!
  subroutine wtf4_r(fname,data)
  character(len=*), intent(in) :: fname
  real(kind=rfp), dimension(1:,1:,1:,1:) :: data
!
  integer :: i,j,k,m
    open(38,file=fname)
    write(38,*) 4, 0     ! The number of dimensions
    do k=1,4
      write(38,*) ubound(data,k), 0
    end do
    do m=1,ubound(data,4)
      do k=1,ubound(data,3)
        do j=1,ubound(data,2)
          do i=1,ubound(data,1)
            write(38,*) data(i,j,k,m), 0
          end do
        end do
      end do
    end do
    close(38)
  end subroutine wtf4_r
!
!
  subroutine wtf1_c(fname,data)
  character(len=*), intent(in) :: fname
  complex(kind=cfp), dimension(1:) :: data
!
  integer :: k
    open(38,file=fname)
    write(38,*) 1, 0     ! The number of dimensions
    write(38,*) ubound(data,1), 0
    do k=1,ubound(data,1)
      write(38,*) real(data(k)),aimag(data(k))
    end do
    close(38)
  end subroutine wtf1_c
!
!
  subroutine wtf2_c(fname,data)
  character(len=*), intent(in) :: fname
  complex(kind=cfp), dimension(1:,1:) :: data
!
  integer :: j,k
    open(38,file=fname)
    write(38,*) 2, 0     ! The number of dimensions
    do k=1,2
      write(38,*) ubound(data,k), 0
    end do
    do k=1,ubound(data,2)
      do j=1,ubound(data,1)
        write(38,*) real(data(j,k)),aimag(data(j,k))
      end do
    end do
    close(38)
  end subroutine wtf2_c
!
!
  subroutine wtf3_c(fname,data)
  character(len=*), intent(in) :: fname
  complex(kind=cfp), dimension(1:,1:,1:) :: data
!
  integer :: i,j,k
    open(38,file=fname)
    write(38,*) 3, 0     ! The number of dimensions
    do k=1,3
      write(38,*) ubound(data,k), 0
    end do
    do k=1,ubound(data,3)
      do j=1,ubound(data,2)
        do i=1,ubound(data,1)
          write(38,*) real(data(i,j,k)),aimag(data(i,j,k))
        end do
      end do
    end do
    close(38)
  end subroutine wtf3_c
!
!
  subroutine wtf4_c(fname,data)
  character(len=*), intent(in) :: fname
  complex(kind=cfp), dimension(1:,1:,1:,1:) :: data
!
  integer :: i,j,k,m
    open(38,file=fname)
    write(38,*) 4, 0     ! The number of dimensions
    do k=1,4
      write(38,*) ubound(data,k), 0
    end do
    do m=1,ubound(data,4)
      do k=1,ubound(data,3)
        do j=1,ubound(data,2)
          do i=1,ubound(data,1)
            write(38,*) real(data(i,j,k,m)),aimag(data(i,j,k,m))
          end do
        end do
      end do
    end do
    close(38)
  end subroutine wtf4_c
!
end module output

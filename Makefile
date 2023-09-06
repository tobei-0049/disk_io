
OS_NAME = $(shell uname -s)

###  This is the default if a platform needs a spaecial compiler please set overwrite
###  variable in the system dependant part...
##CC   = mpicc
CC   = cc
MPI  = -DUSE_MPI

###
##   see http://ftp.sas.com/standards/large.file/x_open.20Mar96.html
###      http://ftp.sas.com/standards/large.file/x_open.20Mar96.html#3.3
#POSIX_LARGE_FILES = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -D_LFS_LARGEFILE=1
POSIX_LARGE_FILES = -D_LARGEFILE_SOURCE=1 -D_LFS_LARGEFILE=1 -D_FILE_OFFSET_BITS=64

ifeq ($(OS_NAME),SunOS)
   SUN_SOLARIS7 = -xarch=v9 -DSOLARIS_7
   ARCH_FLAG = ${SUN_SOLARIS7}
endif

ifeq ($(OS_NAME),sn6715)
  # cray's uname -s print the serial number of the system...
  CRAY_T3E     = -DUSE_MPI -lmpi -g 
  ARCH_FLAG =  ${CRAY_T3E}
  NPROC = 3
endif

ifeq ($(OS_NAME),Linux)
  # just to set ARCH_FLAG to something
  ## ARCH_FLAG = -DLINUX ${POSIX_LARGE_FILES} -Wall ${MPI}
  ## ARCH_FLAG = -DLINUX ${POSIX_LARGE_FILES}  ${MPI}
  CC=mpicc
  ARCH_FLAG = -DLINUX ${POSIX_LARGE_FILES}  ${MPI} 
endif

ifeq ($(OS_NAME),HI-UX/MPP)
  HITACHI      = -DHITACHI -DUSE_MPI -L/usr/local/mpi/lib/ -lmpi
  ARCH_FLAG = ${HITACHI}
endif

ifeq ($(OS_NAME),HP-UX)
  CC             = /usr/bin/cc
  ###jCC             = /usr/local/GNU/bin/gcc
  # MPI_PARALLEL = -DUSE_MPI -lmpi
  HP           = +DD64 ${POSIX_LARGE_FILES}
  ###HP           = ${POSIX_LARGE_FILES}
  ARCH_FLAG = ${HP}
endif


ifeq ($(OS_NAME),IRIX64)
  SGI          =  -g  -64
  ARCH_FLAG = ${SGI} -pedantic
endif
ifeq ($(OS_NAME),IRIX)
  SGI          =  -g  -64
  ARCH_FLAG = ${SGI} -pedantic
endif

ifeq ($(OS_NAME),SUPER-UX)
  # just to set ARCH_FLAG to something
  ARCH_FLAG = -DNEC
endif



CFLAGS   =  -I${HOME}/include -I. -g ${ARCH_FLAG}
LDFLAGS  =  ${ARCH_FLAG}

OBJ = disk.o master.o slave.o steuerfile.o commands.o stuff.o

SRC = ${OBJ:.o=.c}


disk_io:  ${OBJ}
	$(CC) ${LDFLAGS} -o disk_io ${OBJ}


${OBJ}: disk_io.h


lint:
	lint ${ARCH_FLAG} master.c disk.c slave.c steuerfile.c stuff.c

install: disk_io
	cp disk_io ${HOME}/bin
	chmod 711  ${HOME}/bin/disk_io

clean:
	rm -f disk_io *.o 


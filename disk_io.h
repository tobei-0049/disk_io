
#if !defined(__DISK_IO__)
# define __DISK_IO__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>


#if (!defined(_POSIX_SOURCE)&&(defined(__sgi)||defined(sun)))
# define  BOOL  boolean_t
#else
#  if (defined(BOOL) || defined(B_FALSE) || defined(B_TRUE))
#    if (!defined(BOOL))
       typedef int BOOL;
#    endif
#    if (!defined(B_FALSE))
#      define B_FALSE 0
#    endif
#    if (!defined(B_TRUE))
#      define B_TRUE (~B_FALSE)
#    endif
#  else
     typedef enum { B_FALSE = 0, B_TRUE } BOOL;
#  endif
#endif

/*   so war's mal 
#if (!defined(_POSIX_SOURCE)&&(defined(__sgi)||defined(sun)||defined(SX)))
#  include <sys/types.h>
#  define  BOOL  boolean_t
#else
   typedef enum { B_FALSE = 0, B_TRUE } BOOL;
#endif

*/

#if defined(LINUX)
   /* this is definive a problem and wrong but it works... */
#  define  MY_SIZE_T     long long
#else
#  define  MY_SIZE_T     long
#endif

#define  EIN_MB   (1024*1024)
#define  MAX_FILE_SIZE   (500 * 1024)   /*  500 GBytes */
#define  MAX_BUFF_SIZE  (EIN_MB * 4)

#define  MIN_VALUE      LONG_MAX  /* initial value to build minimum */

#if defined(USE_MPI)
#  include <mpi.h>
#  define  MAX_SLAVES   1024
#else
#  define  MAX_SLAVES   20
#endif  /* USE_MPI */


#if defined(LINUX)
# define WR_RES_STR     \
   "(%d) filename:%s buf_size:%lld file_size:%lld sec:%ld u_sec:%ld mb_s:%lf\n"
#else
# define WR_RES_STR     \
   "(%d) filename:%s buf_size:%ld file_size:%ld sec:%ld u_sec:%ld mb_s:%lf\n"
#endif 

#if defined(LINUX)
#define GEN_RES_STR     "(%d) size:%lld count:%d time:%lf\n"
#else
#define GEN_RES_STR     "(%d) size:%ld count:%d time:%lf\n"
#endif

#define REM_RES_STR     "(%d) count:%d time:%lf\n"
#define STA_RES_STR     "(%d) count:%d time:%lf\n"

#define MY_MAX_STR_LEN   1023


typedef enum { PERF_QUITE = 77, PERF_ONCE, PERF_CONTI, PERF_SUM } PERF_OUTPUT;
/*  PERF_QUITE    print nothing
 *  PERF_ONCE     print perf numbers when finished but only numbers
 *  PERF_CONTI    show performance numbers while writing
 *  PERF_SUM      print perfomance numbers in human readable form when finished
 */


typedef enum { GEN_FILES = 49, REM_FILES, STAT_FILES, WRITE_FILE, 
               READ_FILE, EVAL_SCRIPT, END_PROG } COMMAND_ID;

typedef struct {
       char        command[MY_MAX_STR_LEN + 1];
       COMMAND_ID  cmd_id;
       int         n_cpu;
       int         files;
       MY_SIZE_T      size;
       MY_SIZE_T      buffer_size;
       BOOL        fn_id;
       char        prefix[MY_MAX_STR_LEN + 1];
       char        prot[MY_MAX_STR_LEN + 1];
               } COMMAND;

#if defined(LINUX)
#  define CMD_FMT "%d %d %d %lld %lld %d PREFIX=%s  PROT=%s"
#else
#  define CMD_FMT "%d %d %d %ld %ld %d PREFIX=%s  PROT=%s"
#endif

/* ______________________  global variables  _____________________ */

extern int    My_id;
extern pid_t  My_pid;
extern int    Slaves;
extern char   Protokoll_dir[];

extern BOOL Do_debug;

/* ______________________  prototypes _____________________ */

int do_write ( int, char *, MY_SIZE_T );
int write_a_file ( const char *path, const MY_SIZE_T buf_size,
		const MY_SIZE_T file_size, const PERF_OUTPUT mode, const BOOL log );
int read_a_file ( const char *path, MY_SIZE_T buf_size,
		PERF_OUTPUT mode, const BOOL log );

int print_result ( char *str, ... );
int print_info ( char *str, ... );
int my_printf ( char *str, ... );
void Debug ( char *str, ... );
void Error ( char *str, ... );
void abbruch ( char *str, ... );

int do_slave (void);

char *gen_cmd_str ( COMMAND *cmd );
void send_msg ( int proc_nr, char *msg );
int get_msg ( char *buffer, int max_len, int time_out );

int rem_files ( COMMAND *cmd );
int read_file ( COMMAND *cmd );
int write_file ( COMMAND *cmd );
int gen_files ( COMMAND *cmd );
int eval_script ( COMMAND *cmd );
int stat_files ( COMMAND *cmd );

MY_SIZE_T str2size ( char * );
COMMAND *read_command ( char *st_file, BOOL fn_id, FILE **s_hdl );
int init_steuer_file ( char *st_file );

#endif



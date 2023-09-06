
#include "disk_io.h"

#include <sys/wait.h>
#if !defined(HITACHI)
#  if !defined(__hpux)
#    include <sys/select.h>
#  endif /* __hpux */
#else   /* HITACHI */
   /* Hitachi need declaration of the following types */
   extern char *optarg;
   extern int optind, opterr, optopt;
#endif  /* HITACHI */

#if !defined(USE_MPI)
   static  int      Read_fds[MAX_SLAVES];
   static  int      Write_fds[MAX_SLAVES];
   static  pid_t    Slave_pids[MAX_SLAVES];
#endif  /* USE_MPI */

int      Slaves = 0;
int      My_id  = -1;

char     Protokoll_dir[1024] = "/";

BOOL     Do_debug = B_FALSE;

/* ___________________________________________________________________ */

#if !defined(USE_MPI)
static int do_fork ( int my_id )
{
   pid_t ret_val = 0;
   int   pipe_1[2], pipe_2[2];

   if ( pipe ( pipe_1 ) < 0 || pipe ( pipe_2 ) < 0 )
       abbruch ( " can't create a pipe \n" );

   My_id = my_id;

   switch ( (ret_val = fork ()) )
     {
       case -1:
            abbruch ( "can't fork\n" );
          break;
       case   0:           /* slave process */
            close ( pipe_1[1] );
            close ( pipe_2[0] );
            close ( 0 );
            if ( dup ( pipe_1[0] ) != 0 )
                 abbruch ( " can't dup stdout\n" );
            close ( pipe_1[0] );
            close ( 1 );
            if ( dup ( pipe_2[1] ) != 1 )
                abbruch ( " can't dup stdout\n" );
            close ( pipe_2[1] );
            Slaves = 1;  /* needed in script-file evaluation */
            exit ( do_slave () );
          break;
     }

   /*  master process */
   close ( pipe_1[0] );
   close ( pipe_2[1] );

   Read_fds[Slaves]   = pipe_2[0];
   Write_fds[Slaves]  = pipe_1[1];
   Slave_pids[Slaves] = ret_val;
   Slaves++;

   return ( 0 );
}
#endif  /* USE_MPI */

/* ___________________________________________________________________ */

void send_msg ( int proc_nr, char *msg )
{
  char *c_ptr = NULL;
  int   len;

  Debug ( "  send_msg(%d)  to node %d   >>%20s<<\n", My_id, proc_nr, msg );

  proc_nr--;  /* map 0 ... (n-1) auf -1 ... (n-2) */
   if ( (c_ptr =  strchr ( msg, '\n' )) == NULL )
      strcat ( msg, "\n" );
   else
     if ( *(c_ptr+1) != '\0' )
       {
         Error ( " internal error: worng command format >>%s<<\n", msg );
          *(c_ptr+1) = '\0';
       }

   if ( (len = strlen ( msg ) ) < 2 )
     {
       Error ( " internal error: mesg len %d  >>%s<<\n", len, msg );
     }

   if ( proc_nr >= Slaves || proc_nr < -1 )
     {
       Error ( " internal error: write to invalid process\n" );
       return;
     }

#if defined(USE_MPI)
   if ( MPI_Send ( msg, len+1, MPI_CHAR, proc_nr+1, 0, MPI_COMM_WORLD ) != 0 )
	  abbruch ( "Error in MPI_Send" );
#else
   do_write ( (proc_nr == -1 ? 1 : Write_fds[proc_nr]), msg, len );
#endif /* USE_MPI */
}
/* ___________________________________________________________________ */

int get_msg ( char *buffer, int max_len, int time_out )
{
    int            ret_val;
    BOOL           found = B_FALSE;

#if !defined(USE_MPI)
    fd_set         r_fds;
    struct timeval tv;
    int            x;
    int max_fd=0;

    FD_ZERO (&r_fds);
    for ( x = 0; x < Slaves; x++ )
      {
        FD_SET (Read_fds[x], &r_fds);
        if ( Read_fds[x] > max_fd )
           max_fd = Read_fds[x];
      }

    tv.tv_sec  = time_out;
    tv.tv_usec = 0;

    ret_val = select ( max_fd+1, &r_fds, NULL, NULL,
                             (time_out == 0 ? NULL : &tv ) );
    switch ( ret_val )
      {
        case  -1:
            abbruch ( " error in select\n" );
		  break;
        case   0:
          break;  /* nothing to read */
        default:
            for ( x = 0; x < Slaves && found == B_FALSE; x++ )
              {
                if ( FD_ISSET ( Read_fds[x], &r_fds ) )
                  {
                    int n;
                    if ( ( n = read ( Read_fds[x], buffer, max_len-2 ) ) == -1 )
                      abbruch ( " read nach select\n" );
                     buffer[n++] = '\n';
                     buffer[n++] = '\0';
                     found = B_TRUE;
                  }
              }
     }

   if ( found == B_TRUE )
     return ( x );

   return ( -1 );

#else

   MPI_Status status;
   Debug ( " enter funktion get_msg" );

   ret_val = MPI_Recv ( buffer, max_len, MPI_CHAR, MPI_ANY_SOURCE, 0,
						 MPI_COMM_WORLD, &status );
   Debug ( " return in funktion get_msg from node %d", (int)status.MPI_SOURCE );

   return ( 4711 );
#endif

}

/* ___________________________________________________________________ */

char *gen_cmd_str ( COMMAND *cmd )
{
   static char *cmd_buf = NULL;

   if ( cmd_buf == NULL )
    {
      if ( (cmd_buf = (char *)malloc ( sizeof (COMMAND) )) == NULL )
          abbruch ( " can't get memory for cmd packet\n" );
    }

   sprintf ( cmd_buf, CMD_FMT,
               cmd->cmd_id, cmd->n_cpu, cmd->files,
               (MY_SIZE_T) cmd->size,
               (MY_SIZE_T) cmd->buffer_size,
               cmd->fn_id,
               cmd->prefix, cmd->prot );


   Debug ( " command-str is: >>%s%<<\n", cmd_buf );

   return ( cmd_buf );
}

/* ___________________________________________________________________ */


static void do_master ( char *steuer_file )
{
  COMMAND  *cmd, end_cmd;
  char     *end_cmd_str;
  FILE     *s_hdl = NULL;
  int      x;

  Debug ( " anzahl der Slaves: %d\n", Slaves );

    while ( ( cmd = read_command ( steuer_file, B_FALSE, &s_hdl ) ) != NULL )
      {
        Debug ( " read cmd %s  CPUS %d  cmdid: %d\n",
                 cmd->command, cmd->n_cpu, cmd->cmd_id );
        switch ( cmd->cmd_id )
          {
            case   GEN_FILES:
                  gen_files ( cmd );
                break;

            case   REM_FILES:
                  rem_files ( cmd );
                break;

            case   STAT_FILES:
                  stat_files ( cmd );
                break;

            case   WRITE_FILE:
                  write_file ( cmd );
                break;

            case   READ_FILE:
                  read_file ( cmd );
                break;

            case   EVAL_SCRIPT:
                  eval_script ( cmd );
                break;

            default:
                Error ( " unknown command %s (cmd-id: %d)\n", cmd->command, cmd->cmd_id );
          }
      }

   memset ( &end_cmd, 0, sizeof ( end_cmd ) );
   end_cmd.cmd_id = END_PROG;
   end_cmd.prefix[0] = 'x';
   end_cmd.prot[0]   = 'y';
   end_cmd_str = gen_cmd_str ( &end_cmd );

   for ( x = 1; x < Slaves; x++ )
	 send_msg ( x, end_cmd_str );

   fclose ( s_hdl );
}

/* ___________________________________________________________________ */

static void term_clients ( void )
{
#if !defined(USE_MPI)
  int x;

  for ( x = 0; x < Slaves; x++ )
    close ( Write_fds[x] ); /* slave will terminate */
#endif /* USE_MPI */
}

/* ___________________________________________________________________ */

static int do_test_suite_mode ( char *steuer_file )
{
  int   nr_procs;

  Debug ( "do_test_suite_mode steuer_file >>%s<<", steuer_file );

#if !defined(USE_MPI)
   {
   int x, pid;
   int   status;

   nr_procs = init_steuer_file ( steuer_file );

   for ( x = 0; x < nr_procs; x++ )
     if ( do_fork ( x ) != 0 )
         abbruch ( " error in fork\n" );

   do_master ( steuer_file );
   term_clients ();

   Debug ( " wait for children\n" );
   for ( x = 0; x < Slaves; x++)
     {
       pid = waitpid ( 0, &status, 0 );
       Debug ( " child %d returned %d\n", pid, WEXITSTATUS(status));
     }
   }
#else


	if ( My_id == -1 )
	  {
        Debug ( "   in fkt do_test_suite_mode() master part.." );
        nr_procs = init_steuer_file ( steuer_file );
	    do_master ( steuer_file );
        Debug ( "   in fkt do_test_suite_mode() master part finished" );
	  }
	else
	  {
		Slaves = 1;
        Debug ( "   in fkt do_test_suite_mode() slave part - entering do_slave ()" );
	    do_slave ();
        Debug ( "   in fkt do_test_suite_mode() slave part do_slave()  finished" );
	  }

#endif /* USE_MPI */

   return ( 0 );
}
/* ___________________________________________________________________ */

static int do_read_mode ( char *read_file_name, MY_SIZE_T buff_size,
                           MY_SIZE_T size, const BOOL log,
                           const PERF_OUTPUT perf_out )
{
   char *fn;

#if defined(USE_MPI)
   char  fname[FILENAME_MAX+4+8]; /* filename + path + length [+ MPI rank ] */
   memset(fname, 0, FILENAME_MAX+4+8);

		 {
           sprintf ( fname, "./%s.%d", read_file_name, My_id );
		   fn = fname;
		 }
#else
         fn = read_file_name;
#endif /* USE_MPI */

   Debug ( " read a file >>%s<< size %ld  buffer %ld",
		 fn, (long)size, (long)buff_size );
   return ( read_a_file ( fn, buff_size, perf_out, log ) );
}

/* ___________________________________________________________________ */

static int do_creation_mode ( MY_SIZE_T buff_size, MY_SIZE_T size,
							  const BOOL log, const PERF_OUTPUT perf_out )
{
   char  fname[FILENAME_MAX+4+8]; /* filename + path + length [+ MPI rank ] */
   long  n_length;

   memset(fname, 0, FILENAME_MAX+4+8);

   if ( size < EIN_MB )
	 {
#      if defined(USE_MPI)
         sprintf ( fname, "./%ldB.%d", (long)size, My_id );
#      else
         sprintf ( fname, "./%ldB", (long)size );
#      endif /* USE_MPI */
     }
   else
	 {
	   n_length = size / EIN_MB;
#      if defined(USE_MPI)
         sprintf ( fname, "./%ldMB.%d", n_length, My_id );
#      else
         sprintf ( fname, "./%ldMB", n_length );
#      endif /* USE_MPI */
     }

   Debug ( " create a file >>%s<< size %ld  buffer %ld",
		 fname, (long)size, (long)buff_size );
   return ( write_a_file ( fname, buff_size, size, perf_out, log ) );
}

/* ___________________________________________________________________ */

MY_SIZE_T str2size ( char *size_str )
{
   long   l_size;
   MY_SIZE_T buff_size;
   char *c_ptr = NULL;

/* here we should have a atoll function ... */
   if ( (l_size = atol ( size_str ) ) <= 0 )
     return ( -1 );

   buff_size = (MY_SIZE_T)l_size;
   c_ptr = size_str + strlen ( size_str ) -1;
   if ( *c_ptr == 'm' || *c_ptr == 'M' )
      buff_size *= EIN_MB;
   if ( *c_ptr == 'k' || *c_ptr == 'K' )
      buff_size *= 1024;
   if ( *c_ptr == 'g' || *c_ptr == 'G' )
     {
        if ( sizeof ( MY_SIZE_T ) <= 4 ) /* check for 32-bit longs */
          abbruch ( "long data type only 32 bit..." );
        buff_size *= (1024  * EIN_MB);
     }

   Debug ( " stringsize >>%s<< converted to %lld     (l_size: %ld)"
              , size_str, (long long)buff_size, l_size );

   return ( buff_size );
}

/* ___________________________________________________________________ */

static char *usage =
    "USAGE:  disk_io [-b buffer_size] [file_size_in_mb]\n"
    "        disk_io -t test_suite\n"
    "        disk_io -r [-b buffer_size] file\n"
    "                -d         enable debug mode\n"
    "                -l         performance output for each junk into a file\n"
    "                -s         print summary only\n";

int main ( int argc, char *argv[] )
{
   int     c;
   int     errflag = 0;
   BOOL    log = B_FALSE;         /* per default no performance-logging */
   BOOL    read_flag = B_FALSE;   /* per default we write a file */
   PERF_OUTPUT  perf_mon = PERF_CONTI; /* only interactive mode flag for
                                        * continous/summary result output */
   char    read_file_name[FILENAME_MAX+4+8]; /* filename + path + lenght */
   MY_SIZE_T  size = 10, buff_size = EIN_MB;
   char   *test_suite = NULL, *size_str;
   int     b_flag = 0, t_flag = 0;
   int     ret_val;

   memset(read_file_name, 0, FILENAME_MAX+4+8);

#if defined(USE_MPI)
# if defined (_CRAYT3E)
   setlinebuf ( stderr );   /* output to stdout/stderr is not mixed */
# endif
   MPI_Init ( &argc, &argv );
   MPI_Comm_size ( MPI_COMM_WORLD, &Slaves );
   MPI_Comm_rank ( MPI_COMM_WORLD, &My_id );
   My_id--;
#endif /* USE_MPI */

   while ((c = getopt (argc, argv, "dsb:t:r:l")) != -1)
    {
      switch (c)
        {
          case 'd':
				 printf ( "enable debugging\n" );
			     Do_debug = B_TRUE;
			   break;
          case 's':
			     perf_mon = PERF_SUM;
			   break;
          case 'l':
			      log = B_TRUE;
			   break;
          case 'b':
               if (t_flag)
                 errflag++;
               else
                 {
                   size_str = optarg;
                   if ( ( buff_size = str2size ( size_str ) ) < 0 )
                    {
                      Error ( " ERROR negative buff_size size\n" );
                      errflag++;
                     }
                  }
                b_flag++;
             break;
          case 't':
                if (b_flag)
                 errflag++;
               else
                 test_suite = optarg;
               t_flag++;
             break;
          case 'r':
               if (t_flag)
                 errflag++;
               else
                 {
					char *fn;
					int   len_name;
					read_flag = B_TRUE;
					fn = optarg;
					len_name = strlen ( fn );
					if ( len_name == 0 || len_name >= sizeof (read_file_name)-1)
					  abbruch ( " filename too long >>%s<<", fn );
					strcpy ( read_file_name, fn );
                 }
             break;
          case '?':
               errflag++;
             break;
        }
     }

#if defined(USE_MPI)
   Debug ( "start disk_io in MPI-mode rank is: %d", (My_id + 1));
#else  /* USE_MPI */
   Debug ( "start disk_io in sharde memory mode" );
#endif /* USE_MPI */

   Debug ( "errflag:%d t_flag:%d b_flag:%d read_flag:%d  argc:%d  optind:%d",
             errflag, t_flag, b_flag, read_flag, argc, optind );

   if ( errflag > 0 || t_flag > 1 || b_flag > 1 )
       abbruch ( "%s", usage );

   if ( t_flag  && optind != argc )
       abbruch ( "%s", usage );

   if ( b_flag )
     if ( (argc - optind != 1 && read_flag == B_FALSE )
	      || (argc - optind != 0 && read_flag == B_TRUE ) )
       abbruch ( "%s", usage );

   for ( ; optind < argc; optind++)
      if ( ( size = str2size ( argv[optind] ) ) < 0 )
          abbruch ( " ERROR negative file size \n" );

   if ( t_flag )
     ret_val = do_test_suite_mode ( test_suite );
   else
     if ( read_flag == B_FALSE )
	    ret_val = do_creation_mode ( buff_size, size, log, perf_mon );
	 else
	    ret_val = do_read_mode(read_file_name, buff_size, size, log, perf_mon);

#if defined(USE_MPI)
   MPI_Barrier ( MPI_COMM_WORLD );
   MPI_Finalize ();
#endif /* USE_MPI */

   return ( ret_val );
}


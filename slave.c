
#include "disk_io.h"


pid_t    My_pid = 4711;
extern   int My_id;

static void do_eval_script ( COMMAND *cmd );

/* ___________________________________________________________________ */

static void get_time ( struct timeval *tv )
{
   struct timezone tz;

   tz.tz_minuteswest = 0;
   tz.tz_dsttime     = 0;

   if ( gettimeofday ( tv, &tz ) == -1 )
     Error ( " can't get time by gettimeofday()\n" );

   return;
}

/* ___________________________________________________________________ */

static double tv_2_sec ( struct timeval *tv_start, struct timeval *tv_end )
{
   double sec;

   sec = ((double)( tv_end->tv_sec - tv_start->tv_sec )) +
             (( (double)(tv_end->tv_usec - tv_start->tv_usec)) / 1000000 );
  
   return ( sec );
} 

/* ___________________________________________________________________ */

static int str_2_cmd ( char *str, COMMAND *cmd )
{
   MY_SIZE_T  size, b_size;

   if ( sscanf ( str, CMD_FMT,
               (int *)&(cmd->cmd_id),
               &(cmd->n_cpu),
               &(cmd->files),
               &size,
               &b_size,
               (int *)&(cmd->fn_id),
               cmd->prefix,
               cmd->prot ) != 8 ) {
       abbruch ( "evaluation of command >>%s<< failed", str );
   }

   cmd->size        = size;
   cmd->buffer_size = b_size;

   strncpy ( cmd->command, str, MY_MAX_STR_LEN );
   cmd->command[MY_MAX_STR_LEN] = '\0';


   Debug ( " evaluation of >>%s<< successful\n", str );

   return ( 0 );
}

/* ___________________________________________________________________ */

static void do_rem_files ( COMMAND  *cmd )
{
   int   x;
   char  full_name[1024];
   struct timeval tv_beg, tv_end;

   get_time ( &tv_beg );

   for ( x = 0; x < cmd->files; x++ )
     { 
       sprintf ( full_name, "%s.%d.%d", cmd->prefix, My_id, x );
       if ( unlink ( full_name ) != 0 )
         {
            Error ( " error on removing file %s\n", full_name );
            return;
         }
     }

   get_time ( &tv_end );

   /*
	  Hier gibt's ein problem wenn in der MPI umgebung die Ausgabe innerhalb
	  eines eval_script gerufen wird, darf keine ausgabe erfolgen. Die Ausgabe
	  geht bei MPI immer an Rank 0 was aber bei rekursion (eval_script) schief
	  gehen muss...
   */
#if !defined(USE_MPI)
   print_result (REM_RES_STR, My_pid, cmd->files, tv_2_sec ( &tv_beg,&tv_end ));
#endif
} 

/* ___________________________________________________________________ */

static void do_stat_files ( COMMAND  *cmd )
{
   int   x;
   char  full_name[1024];
   struct timeval tv_beg, tv_end;
   struct stat file_status;

   get_time ( &tv_beg );

   for ( x = 0; x < cmd->files; x++ )
     { 
       sprintf ( full_name, "%s.%d.%d", cmd->prefix, My_id, x );
       if ( stat ( full_name, & file_status) != 0 )
         {
            Error ( " error on getting file status %s\n", full_name );
            return;
         }
     }

   get_time ( &tv_end );

   /*
	  Hier gibt's ein problem wenn in der MPI umgebung die Ausgabe innerhalb
	  eines eval_script gerufen wird, darf keine ausgabe erfolgen. Die Ausgabe
	  geht bei MPI immer an Rank 0 was aber bei rekursion (eval_script) schief
	  gehen muss...
   */
#if !defined(USE_MPI)
   print_result (STA_RES_STR, My_pid, cmd->files, tv_2_sec ( &tv_beg,&tv_end ));
#endif
} 

/* ___________________________________________________________________ */

static void do_gen_files ( COMMAND *cmd )
{
   int   x;
   char  full_name[1024];
   struct timeval tv_beg, tv_end;
   
   get_time ( &tv_beg );

   for ( x = 0; x < cmd->files; x++ )
     { 
       sprintf ( full_name, "%s.%d.%d", cmd->prefix, My_id, x );
       if ( write_a_file ( full_name, cmd->buffer_size, cmd->size,
						   PERF_QUITE, B_FALSE )
             != 0 )
         {
            Error ( " error on writing\n" );
            return;
         }
     }

   get_time ( &tv_end );

   Debug ( " vor print_result cmd dump: %s  prefix: %s   prot: %s",
				   cmd->command, cmd->prefix, cmd->prot );
   
   /*
	  Hier gibt's ein problem wenn in der MPI umgebung die Ausgabe innerhalb
	  eines eval_script gerufen wird, darf keine ausgabe erfolgen. Die Ausgabe
	  geht bei MPI immer an Rank 0 was aber bei rekursion (eval_script) schief
	  gehen muss...
   */ 
#if !defined(USE_MPI)
   print_result ( GEN_RES_STR, My_pid, cmd->size, cmd->files,
                tv_2_sec (&tv_beg, &tv_end) );
#endif
} 

/* ___________________________________________________________________ */

static void do_write_file ( COMMAND *cmd )
{
   char  full_name[1024];
  
   sprintf ( full_name, "%s.%d.%d", cmd->prefix, My_id, 0 );

   Debug ( "write file >>%s<< size %lld buf_size %lld\n",
    full_name, (long long)cmd->size, (long long)cmd->buffer_size );

   if ( write_a_file ( full_name, cmd->buffer_size, cmd->size, 
					  PERF_ONCE, B_FALSE )
           != 0 )
     {
       Error ( " error on writing\n" );
       return;
     }
} 

/* ___________________________________________________________________ */

static void do_read_file ( COMMAND *cmd )
{
   char  full_name[1024];

   sprintf ( full_name, "%s.%d.%d", cmd->prefix, 
             (cmd->fn_id == B_TRUE ? My_id : 0 ), 0 );

   Debug ( " do_read_file()  read file: >>%s<<\n", full_name );

   if ( read_a_file ( full_name, cmd->buffer_size, PERF_ONCE, B_FALSE )
          == -1 )
     {
       Error ( " error on reading >>%s<<\n", full_name );
       return;
     }
} 

/* ___________________________________________________________________ */

static int eval_cmd ( COMMAND *cmd )
{
  Debug ( "  eval_cmd  cmd_id %d\n", cmd->cmd_id );
   switch ( cmd->cmd_id )
     {
       case GEN_FILES:
                Debug ( " do_gen_files >>%s<<", cmd->command );
                do_gen_files ( cmd );
             break;

       case REM_FILES:
                Debug ( " do_rem_files >>%s<<", cmd->command );
                do_rem_files ( cmd );
             break;

       case STAT_FILES:
                Debug ( " do_stat_files >>%s<<", cmd->command);
                do_stat_files ( cmd );
             break;

       case WRITE_FILE:
                Debug ( " do_write_files >>%s<<", cmd->command );
                do_write_file ( cmd );
             break;

       case READ_FILE:
                Debug ( " do_read_files >>%s<<", cmd->command );
                do_read_file ( cmd );
             break;

       case EVAL_SCRIPT:
                Debug ( " do_eval_files >>%s<<", cmd->command );
                do_eval_script ( cmd );
             break;

       case END_PROG:
                Debug ( " END_PROG" );
                return ( 1 );
             /* break; */

       default:
            Error ( " unknown command >>%d<<\n", cmd->cmd_id );
     }

   Debug ( " eval_cmd return 0 arg war >>%s<<", cmd->command );
   return ( 0 );
}

/* ___________________________________________________________________ */

static int eval_str ( char *str )
{
   COMMAND cmd;

   Debug ( " (%d) got command >>%s<<\n", My_pid, str );
   if ( str_2_cmd ( str, &cmd ) != 0 )
      abbruch ( "vor exit \n" );

   return ( eval_cmd ( &cmd ) );
}

/* ___________________________________________________________________ */

static void do_eval_script ( COMMAND *cmd )
{
   FILE  *s_hdl = NULL;
   int   save_stdout;
   struct timeval tv_beg, tv_end;
   COMMAND *cmd2;
   
   if ( ( save_stdout = dup ( 1 ) ) == -1 )
       abbruch ( " can't dup stdout\n" );
   close ( 1 );
   
   if ( open ( "/dev/null", O_WRONLY ) != 1 )
      abbruch ( " can't redircet stdout\n" );
   
   Debug ( " start in function do_eval_script()" );
   
   get_time ( &tv_beg );

   while ( ( cmd2 = read_command ( cmd->prefix, cmd->fn_id, &s_hdl ) ) != NULL )
      {
        Debug ( " (%d) read cmd %s  CPUS %d  cmdid: %d\n",
                   My_id, cmd2->command, cmd2->n_cpu, cmd2->cmd_id );
        my_printf ( "das sollte nicht gesehen werden >>%s<<\n", cmd2->command );
        eval_cmd ( cmd2 );
      }

   get_time ( &tv_end );

   close ( 1 );
   if ( dup ( save_stdout ) != 1 )
       abbruch ( " reopen of stdout failed\n" );
   close ( save_stdout );

   print_result ( "time:%lf\n", tv_2_sec ( &tv_beg, &tv_end ) );
} 

/* ___________________________________________________________________ */

int do_slave ( void )
{
   int n;
   int   status = 0;

   My_pid = getpid();
   Debug ( " hi hier ist proc %d in function do_slave()", My_pid );

   do {
        char buffer[1000];
        memset ( buffer, 0 , 1000 );

#if !defined(USE_MPI)

        n = scanf ( "%[^\n]", buffer );
        if ( n == 0 )
         getchar ();
        if ( n == EOF )
          status = 1;
#else
	    get_msg ( buffer, 1000, 0 );
		if ( (n = strlen ( buffer ) ) == 0 )
		  status = 1;
        Debug ( " get message in funktion do_slave() len was %d", n );
#endif
        if ( n > 0 )
          {
            if ( eval_str ( buffer )  == 1 )
			  status = 1;
          }
      } while ( status == 0 );
   
   Debug ( " hi slave finishes" );
   fflush ( stderr );
   return ( 0 );
}


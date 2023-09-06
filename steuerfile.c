#include "disk_io.h"



int init_steuer_file ( char *st_file )
{
   FILE  *s_hdl;
   char   line[1025], dir_name[1025];
   int    args, line_cnt = 0, n_cpu = -47;

   memset(line, 0, 1025);
   memset(dir_name, 0, 1025);

   if ( (s_hdl = fopen ( st_file, "r" )) == NULL )
       abbruch ( " can't open file >>%s<<\n", st_file );

   while ( ( args = fscanf ( s_hdl, "%1023[^\n]\n", line ) ) != EOF )
     {
       line_cnt++;

       if ( args == 0 )
         {
           getc ( s_hdl );
           continue;
         }

       if ( line[0] == '#' )
         continue;

       if ( sscanf ( line, "MAX_PROC  %d", &n_cpu ) == 1 )
           Debug ( " found MAX_PROC in line %d  n_cpu = %d\n",
                 line_cnt, n_cpu );

       if ( sscanf ( line, "PROTOKOLL_DIR %s", dir_name ) == 1 )
         {
           Debug ( " found PROTOKOLL_DIR >>%s<<\n", dir_name );
           strcpy ( Protokoll_dir, dir_name );
         }
    }

  fclose ( s_hdl );

  if ( n_cpu == -47 )
      abbruch ( " MAX_PROC not defined in %s\n", st_file );
  if ( n_cpu <= 0 || n_cpu > MAX_SLAVES )
      abbruch ( " MAX_PROC out of range  (0<MAX_PROC<=%d)\n", MAX_SLAVES );

  return ( n_cpu );
}

/* ____________________________________________________________________ */

static void check_cmd ( COMMAND *cmd )
{
   if ( cmd->n_cpu > Slaves )
       abbruch ( " too many cpu required CPUs: %d  Procs: %d\n",
           cmd->n_cpu, Slaves );
}

/* ____________________________________________________________________ */

static void parse_rem_file ( COMMAND  *cmd, char *args, char *line )
{
  int  f,  n;
  char     prot[1025], prefix[80];

  memset(prot, 0, 1025);
  memset(prefix, 0, 80);

  if ( sscanf ( args,
           "%d N_PROC %d  PREFIX=%s PROTOKOLL_FILE=%s",
                         &f, &n, prefix, prot ) != 4 )
      abbruch ( " error in command >>%s<<\n", line );

  cmd->cmd_id = REM_FILES;
  cmd->n_cpu  = n;
  cmd->files  = f;
  cmd->size   = 0;
  cmd->fn_id  = B_TRUE;
  cmd->buffer_size = 0;
  strcpy   ( cmd->prefix, prefix );

  if ( strlen ( prot ) == 0 )
     strcpy ( cmd->prot, "/dev/null" );
  else
     strcpy ( cmd->prot, prot );
}

/* ____________________________________________________________________ */

static void parse_stat_file ( COMMAND  *cmd, char *args, char *line )
{
  int  f,  n;
  char     prot[1025], prefix[80];

  memset(prot, 0, 1025);
  memset(prefix, 0, 80);

  if ( sscanf ( args,
           "%d N_PROC %d  PREFIX=%s PROTOKOLL_FILE=%s",
                         &f, &n, prefix, prot ) != 4 )
      abbruch ( " error in command >>%s<<\n", line );

  cmd->cmd_id = STAT_FILES;
  cmd->n_cpu  = n;
  cmd->files  = f;
  cmd->size   = 0;
  cmd->fn_id  = B_TRUE;
  cmd->buffer_size = 0;
  strcpy   ( cmd->prefix, prefix );

  if ( strlen ( prot ) == 0 )
     strcpy ( cmd->prot, "/dev/null" );
  else
     strcpy ( cmd->prot, prot );
}

/* ____________________________________________________________________ */

static void parse_gen_file ( COMMAND  *cmd, char *gen_args, char *line )
{
  int  f,  n;
  char     l_str[100], prot[1025], bs_str[1025], prefix[80];

  memset(l_str, 0, 100);
  memset(prot, 0, 1025);
  memset(bs_str, 0, 1025);
  memset(prefix, 0, 80);

  if ( sscanf ( gen_args,
           "%d N_PROC %d  SIZE=%s BS=%s PREFIX=%s PROTOKOLL_FILE=%s",
                         &f, &n, l_str, bs_str, prefix, prot ) != 6 )
      abbruch ( " error in command >>%s<<\n", line );

  cmd->cmd_id = GEN_FILES;
  cmd->n_cpu  = n;
  cmd->files  = f;
  cmd->size   = str2size ( l_str );
  cmd->buffer_size = str2size ( bs_str );
  cmd->fn_id  = B_TRUE;
  strcpy   ( cmd->prefix, prefix );

  if ( strlen ( prot ) == 0 )
     strcpy ( cmd->prot, "/dev/null" );
  else
     strcpy ( cmd->prot, prot );

}

/* ____________________________________________________________________ */

static void parse_read_file ( COMMAND  *cmd, char *gen_args, char *line )
{
  int    n;
  char     prot[1025], bs_str[1025], prefix[80];
  char  *c_ptr;

  memset(prot, 0, 1025);
  memset(bs_str, 0, 1025);
  memset(prefix, 0, 80);

  if ( sscanf ( gen_args,
           "N_PROC %d  BS=%s PREFIX=%s PROTOKOLL_FILE=%s",
                          &n, bs_str, prefix, prot ) != 4 )
      abbruch ( " error in command >>%s<<\n", line );

  cmd->cmd_id = READ_FILE;
  cmd->n_cpu  = n;
  cmd->files  = 1;
  cmd->size   = -1;
  cmd->buffer_size = str2size ( bs_str );
  strcpy   ( cmd->prefix, prefix );
  if ( (c_ptr = strstr ( cmd->prefix, "###" )) != NULL )
    {
      if ( c_ptr == cmd->prefix + strlen ( cmd->prefix ) - strlen ( "###" ) )
       {
         Debug ( "found  ### mark: set proc_nr in file-name\n" );
         cmd->fn_id = B_TRUE;
         *c_ptr = '\0';
       }
      else
       {
         Error ( "found  ### mark, but not at end of prefix... ignored\n" );
         cmd->fn_id = B_FALSE;
       }
    }
  else
    {
      Debug ( " don't set file-id in file name\n" );
      cmd->fn_id = B_FALSE;
    }


  if ( strlen ( prot ) == 0 )
     strcpy ( cmd->prot, "/dev/null" );
  else
     strcpy ( cmd->prot, prot );

}

/* ____________________________________________________________________ */

static void parse_write_file ( COMMAND  *cmd, char *gen_args, char *line )
{
  int    n;
  char     l_str[100], prot[1025], bs_str[1025], prefix[80];

  memset(l_str, 0, 100);
  memset(prot, 0, 1025);
  memset(bs_str, 0, 1025);
  memset(prefix, 0, 80);

  if ( sscanf ( gen_args,
           "N_PROC %d  SIZE=%s BS=%s PREFIX=%s PROTOKOLL_FILE=%s",
                          &n, l_str, bs_str, prefix, prot ) != 5 )
      abbruch ( " error in command >>%s<<\n", line );

  cmd->cmd_id = WRITE_FILE;
  cmd->n_cpu  = n;
  cmd->files  = 1;
  cmd->size   = str2size ( l_str );
  cmd->buffer_size = str2size ( bs_str );

  cmd->fn_id  = B_TRUE;

  strcpy   ( cmd->prefix, prefix );

  if ( strlen ( prot ) == 0 )
     strcpy ( cmd->prot, "/dev/null" );
  else
     strcpy ( cmd->prot, prot );

}

/* ____________________________________________________________________ */
/* this function is used by slave and master process
 * each slaves reads the same commands
 * this means the slaves are "synchron" by the buffering of the OS
 * ____________________________________________________________________ */
static int parse_eval_script ( COMMAND  *cmd, char *args, char *line )
{
  static   int  script_recursion = 0;
  char     script_file[1025], prot[1025];
  int      n;
  char    *c_ptr;

  memset(script_file, 0, 1025);
  memset(prot, 0, 1025);

  if ( ++script_recursion > 10 )
    {
      Error ( " max recursion level reached (%d) returning...\n",
             script_recursion );
      return ( 1 );
    }

  if ( sscanf ( args, "%s N_PROC %d PROTOKOLL_FILE=%s",
           script_file, &n, prot ) != 3 )
      abbruch ( " error in command >>%s<<\n", line );

  cmd->cmd_id = EVAL_SCRIPT;
  cmd->n_cpu  = n;
  cmd->files  = 0;
  cmd->size   = 0;
  cmd->buffer_size = 0;

  strcpy   ( cmd->prefix, script_file );
  if ( (c_ptr = strstr ( cmd->prefix, "###" )) != NULL )
    {
      if ( c_ptr == cmd->prefix + strlen ( cmd->prefix ) - strlen ( "###" ) )
       {
         Debug ( "found  ### mark: set proc_nr in file-name\n" );
         cmd->fn_id = B_TRUE;
         *c_ptr = '\0';
       }
      else
       {
         Error ( "found  ### mark, but not at end of prefix... ignored\n" );
         cmd->fn_id = B_FALSE;
       }
    }
  else
    {
      Debug ( " don't set file-id in file name\n" );
      cmd->fn_id = B_FALSE;
    }

  if ( strlen ( prot ) == 0 )
     strcpy ( cmd->prot, "/dev/null" );
  else
     strcpy ( cmd->prot, prot );

  return ( 0 );
}

/* ____________________________________________________________________ */

COMMAND *read_command ( char *st_file, BOOL fn_id, FILE **s_hdl )
{
   char   line[1025];
   int    args = 0, line_cnt = 0;
   BOOL   status = B_FALSE;
   static COMMAND cmd = {"", 0, 0, 0, 0, 0, B_FALSE, "", ""};
   char token[1024], rest[1024];
   char parl_st_file[1025];

   memset(line, 0, 1025);
   memset(token, 0, 1024);
   memset(rest, 0, 1024);
   memset(parl_st_file, 0, 1025);

   Debug ( "read_command(%d) file: >>%s<< fn_id: %d", My_id, st_file, fn_id );
   if ( *s_hdl == NULL )
     {

       if ( fn_id == B_FALSE )
         strcpy ( parl_st_file, st_file );
       else
         sprintf ( parl_st_file, "%s%d", st_file, My_id+1 );

       Debug ( "read_command(%d) open file: >>%s<<", My_id, parl_st_file );
       if ( (*s_hdl = fopen ( parl_st_file, "r" )) == NULL )
         abbruch ( " can't open >>%s<<\n", parl_st_file );
     }

   while ( status == B_FALSE &&
           ( args = fscanf ( *s_hdl, "%1023[^\n]\n", line ) ) != EOF )
     {


       line_cnt++;

       if ( args == 0 )
         {
           getc ( *s_hdl );
           continue;
         }

       if ( line[0] == '#' )
         continue;

	   Debug ( " in fkt read_command() line: >>%s<<", line );
       if ( ( args = sscanf ( line, "%s %[^\n]", token, rest ) ) == 2 )
         {
           if ( strcmp ( token, "GEN_FILES" ) == 0 )
             {
			         Debug ( " in fkt read_command() rufe parse_gen_file ()..." );
               strcpy ( cmd.command, token );
               parse_gen_file ( &cmd, rest, line );
               status     = B_TRUE;
             }

           else if ( strcmp ( token, "REM_FILES" ) == 0 )
             {
			         Debug ( " in fkt read_command() rufe parse_rem_file ()..." );
               strcpy ( cmd.command, token );
               parse_rem_file ( &cmd, rest, line );
               status     = B_TRUE;
             }

           else if ( strcmp ( token, "STAT_FILES" ) == 0 )
             {
			         Debug ( " in fkt read_command() rufe parse_stat_file ()..." );
               strcpy ( cmd.command, token );
               parse_stat_file ( &cmd, rest, line );
               status     = B_TRUE;
             }

           else if ( strcmp ( token, "WRITE_FILE" ) == 0 )
             {
			         Debug ( " in fkt read_command() rufe parse_write_file ()..." );
               strcpy ( cmd.command, token );
               parse_write_file ( &cmd, rest, line );
               status     = B_TRUE;
             }

           else if ( strcmp ( token, "READ_FILE" ) == 0 )
             {
			         Debug ( " in fkt read_command() rufe parse_read_file ()..." );
               strcpy ( cmd.command, token );
               parse_read_file ( &cmd, rest, line );
               status     = B_TRUE;
             }
           else if ( strcmp ( token, "EVAL_SCRIPT" ) == 0 )
             {
			         Debug ( " in fkt read_command() rufe parse_eval_script ()..." );
               strcpy ( cmd.command, token );
               if ( parse_eval_script ( &cmd, rest, line )  == 0 )
                 status  = B_TRUE;
             }
           else if ( (strcmp ( token, "MAX_PROC" ) == 0) || (strcmp ( token, "PROTOKOLL_DIR" ) == 0) )
             {
               Debug (" in fkt read_command() ignoriere token: %s", token);
               status = B_FALSE;
             }
           else
             {
               abbruch( " in fkt read_command() unbekannter token: %s", token );
               status = B_FALSE;
             }
         }

    }


  if ( args == EOF )
    return ( NULL );

  check_cmd ( &cmd );

  return ( &cmd );
}



#include "disk_io.h"

/* ___________________________________________________________________ */

int rem_files ( COMMAND *cmd )
{
    char  *cmd_str;
    char res_str[1024] = "noch nie was gelesen";
    double    results[MAX_SLAVES];
    int  x;

    cmd_str = gen_cmd_str ( cmd );

    for ( x = 0; x < cmd->n_cpu; x++ )
      send_msg ( x+1, cmd_str );

    for ( x = 0; x < cmd->n_cpu; x++ )
      {
        double  res;
        (void)get_msg ( res_str, 1000, 0 );
        Debug ( " rem_files() got result >>%s<<\n", res_str );

        if ( sscanf ( res_str, "%*s %*s time:%lf", &res ) != 1 )
          {
            Error ( " error in eval result >>%s<<\n", res_str );
            res = -1;
          }
        else
            results[x] = res;

      }

    if ( strcmp ( cmd->prot, "/dev/null" ) != 0 )
      {
        FILE  *p_file;
        char   p_f_name[1024];
        double min, max, sum;

        sprintf ( p_f_name, "%s/%s", Protokoll_dir, cmd->prot );

        if ( ( p_file = fopen ( p_f_name, "a+" ) ) == NULL )
            abbruch ( " can't open file >>%s<<\n", p_f_name );

        min = MIN_VALUE;
        max = 0;
        sum = 0;

        for ( x = 0; x < cmd->n_cpu; x++ )
          {
            sum += results[x];
            if ( results[x] < min )
               min = results[x];
            if ( results[x] > max )
               max = results[x];
          }

           /* format:
                N_cpus  avg  min max  size nr_of_files
                avg min max in files_created/second */
        fprintf ( p_file, "%d  %f %f %f  %lld %d\n",
                  cmd->n_cpu, (float)(cmd->files*cmd->n_cpu)/max,
                  (float)(cmd->files/max), (float)(cmd->files/min),
                  (long long)cmd->size, cmd->files );

        fclose ( p_file );
     }

   return ( 0 );
}

/* ___________________________________________________________________ */

int stat_files ( COMMAND *cmd )
{
    char  *cmd_str;
    char res_str[1024] = "noch nie was gelesen";
    double    results[MAX_SLAVES];
    int  x;

    cmd_str = gen_cmd_str ( cmd );

    for ( x = 0; x < cmd->n_cpu; x++ )
      send_msg ( x+1, cmd_str );

    for ( x = 0; x < cmd->n_cpu; x++ )
      {
        double  res;
        (void)get_msg ( res_str, 1000, 0 );
        Debug ( " stat_files() got result >>%s<<\n", res_str );

        if ( sscanf ( res_str, "%*s %*s time:%lf", &res ) != 1 )
          {
            Error ( " error in eval result >>%s<<\n", res_str );
            res = -1;
          }
        else
            results[x] = res;

      }

    if ( strcmp ( cmd->prot, "/dev/null" ) != 0 )
      {
        FILE  *p_file;
        char   p_f_name[1024];
        double min, max, sum;

        sprintf ( p_f_name, "%s/%s", Protokoll_dir, cmd->prot );

        if ( ( p_file = fopen ( p_f_name, "a+" ) ) == NULL )
            abbruch ( " can't open file >>%s<<\n", p_f_name );

        min = MIN_VALUE;
        max = 0;
        sum = 0;

        for ( x = 0; x < cmd->n_cpu; x++ )
          {
            sum += results[x];
            if ( results[x] < min )
               min = results[x];
            if ( results[x] > max )
               max = results[x];
          }

           /* format:
                N_cpus  avg  min max  size nr_of_files
                avg min max in files_created/second */
        fprintf ( p_file, "%d  %f %f %f  %lld %d\n",
                  cmd->n_cpu, (float)(cmd->files*cmd->n_cpu)/max,
                  (float)(cmd->files/max), (float)(cmd->files/min),
                  (long long)cmd->size, cmd->files );

        fclose ( p_file );
     }

   return ( 0 );
}

/* ___________________________________________________________________ */

int read_file ( COMMAND *cmd )
{
    char *cmd_str;
    char res_str[1024] = "noch nie was gelesen";
    double    results[MAX_SLAVES];
    long long sizes[MAX_SLAVES];
    int  x;

    cmd_str = gen_cmd_str ( cmd );

    for ( x = 0; x < cmd->n_cpu; x++ )
      send_msg ( x+1, cmd_str );

    for ( x = 0; x < cmd->n_cpu; x++ )
      {
        double  res;
        long long size;

        (void) get_msg ( res_str, 1000, 0 );
        Debug ( " read_file() got result >>%s<<\n", res_str );

        if ( sscanf ( res_str,
                 "%*s %*s %*s file_size:%lld %*s %*s mb_s:%lf",
                    &size, &res ) != 2 )
          {
            Error ( " error in eval result >>%s<<\n", res_str );
            res = -1;
          }
        else
          {
            results[x] = res;
            sizes[x]   = size;
          }
      }

    if ( strcmp ( cmd->prot, "/dev/null" ) != 0 )
      {
        FILE  *p_file;
        char   p_f_name[1024];
        double min, max, sum;

        sprintf ( p_f_name, "%s/%s", Protokoll_dir, cmd->prot );

        if ( ( p_file = fopen ( p_f_name, "a+" ) ) == NULL )
            abbruch ( " can't open file >>%s<<\n", p_f_name );
        min = MIN_VALUE;
        max = 0;
        sum = 0;

        for ( x = 0; x < cmd->n_cpu; x++ )
          {
            sum += results[x];
            if ( results[x] < min )
               min = results[x];
            if ( results[x] > max )
               max = results[x];
            if ( sizes[0] != sizes[x] ) /* check for same file size */
              abbruch ( " error in read test: not always read same size for %s",
                  cmd->prefix );
          }

           /* format:
                N_cpus  sum  min max  size nr_of_files
                sum min max in files_created/second */
        fprintf ( p_file, "%d  %f %f %f  %lld %lld\n",
                  cmd->n_cpu, (float)sum, (float)min, (float)max,
                  sizes[0], (long long)cmd->buffer_size );
        fclose ( p_file );
     }

   return ( 0 );
}

/* ___________________________________________________________________ */

int write_file ( COMMAND *cmd )
{
    char *cmd_str;
    char res_str[1024] = "noch nie was gelesen";
    double    results[MAX_SLAVES];
    int  x;

	Debug ( " in fkt write_file()" );
	cmd_str = gen_cmd_str ( cmd );

    for ( x = 0; x < cmd->n_cpu; x++ )
      send_msg ( x+1, cmd_str );
	Debug ( " in fkt write_file    alle Kommandos gesendet ... ()" );

    for ( x = 0; x < cmd->n_cpu; x++ )
      {
        double  res;
        (void) get_msg ( res_str, 1000, 0 );
        Debug ( " write_file() got result >>%s<<\n", res_str );

        if ( sscanf ( res_str,
                 "%*s %*s %*s %*s %*s %*s mb_s:%lf", &res ) != 1 )
          {
            Error ( " error in eval result >>%s<<\n", res_str );
            res = -1;
          }
        else
            results[x] = res;
      }

    if ( strcmp ( cmd->prot, "/dev/null" ) != 0 )
      {
        FILE  *p_file;
        char   p_f_name[1024];
        double min, max, sum;

        sprintf ( p_f_name, "%s/%s", Protokoll_dir, cmd->prot );

        if ( ( p_file = fopen ( p_f_name, "a+" ) ) == NULL )
            abbruch ( " can't open file >>%s<<\n", p_f_name );
        min = MIN_VALUE;
        max = 0;
        sum = 0;

        for ( x = 0; x < cmd->n_cpu; x++ )
          {
            sum += results[x];
            if ( results[x] < min )
               min = results[x];
            if ( results[x] > max )
               max = results[x];
          }

           /* format:
                N_cpus  sum  min max  size nr_of_files
                sum min max in files_created/second */
        fprintf ( p_file, "%d  %f %f %f  %lld %lld\n",
                  cmd->n_cpu, (float)sum, (float)min, (float)max,
                  (long long)cmd->size, (long long)cmd->buffer_size );
        fclose ( p_file );
	Debug ( " resultfile is >>%s<<", p_f_name );
	Debug ( "  result is " );
        Debug ( "%d  %f %f %f  %lld %lld\n",
                  cmd->n_cpu, (float)sum, (float)min, (float)max,
                  (long long)cmd->size, (long long)cmd->buffer_size );

     }

	Debug ( " end in fkt write_file()" );
   return ( 0 );
}


/* ___________________________________________________________________ */

int gen_files ( COMMAND *cmd )
{
    char *cmd_str;
    char res_str[1024] = "noch nie was gelesen";
    double    results[MAX_SLAVES];
    int  x;

	Debug ( " in fkt gen_files()" );
	cmd_str = gen_cmd_str ( cmd );

    for ( x = 0; x < cmd->n_cpu; x++ )
      send_msg ( x+1, cmd_str );
	Debug ( " in fkt gen_files    alles geschrieben... ()" );

    for ( x = 0; x < cmd->n_cpu; x++ )
      {
        double  res;
        (void) get_msg ( res_str, 1000, 0 );
        Debug ( " gen_files() got result >>%s<<\n", res_str );

        if ( sscanf ( res_str, "%*s %*s %*s time:%lf", &res ) != 1 )
          {
            Error ( " error in eval result >>%s<<\n", res_str );
            res = -1;
          }
        else
            results[x] = res;
      }

    if ( strcmp ( cmd->prot, "/dev/null" ) != 0 )
      {
        FILE  *p_file;
        char   p_f_name[1024];
        double min, max, sum;

        sprintf ( p_f_name, "%s/%s", Protokoll_dir, cmd->prot );

        if ( ( p_file = fopen ( p_f_name, "a+" ) ) == NULL )
            abbruch ( " can't open file >>%s<<\n", p_f_name );
        min = MIN_VALUE;
        max = 0;
        sum = 0;

        for ( x = 0; x < cmd->n_cpu; x++ )
          {
            sum += results[x];
            if ( results[x] < min )
               min = results[x];
            if ( results[x] > max )
               max = results[x];
          }

           /* format:
                N_cpus  average  min max  size nr_of_files
                average min max in files_created/second */
        fprintf ( p_file, "%d  %f %f %f  %lld %d\n",
                  cmd->n_cpu, (float)(cmd->files*cmd->n_cpu)/max,
                  (float)cmd->files/max,
                  (float)cmd->files/min, (long long)cmd->size, cmd->files );
        fclose ( p_file );
	Debug ( " resultfile is >>%s<<", p_f_name );
	Debug ( "  result is " );
        Debug ( "%d  %f %f %f  %lld %lld\n",
                  cmd->n_cpu, (float)(cmd->files*cmd->n_cpu)/max,
                  (float)cmd->files/max,
                  (float)cmd->files/min, (long long)cmd->size, cmd->files );

     }

	Debug ( " end in fkt gen_files()" );
   return ( 0 );
}

/* ___________________________________________________________________ */

int eval_script ( COMMAND *cmd )
{
    char *cmd_str;
    char res_str[1024] = "noch nie was gelesen";
    double    results[MAX_SLAVES];
    int  x;

    cmd_str = gen_cmd_str ( cmd );

    for ( x = 0; x < cmd->n_cpu; x++ )
	  {
        Debug ( " send_msg to node %d   of %d nodes\n", x+1, cmd->n_cpu );
        send_msg ( x+1, cmd_str );
	  }


    for ( x = 0; x < cmd->n_cpu; x++ )
      {
        double  res;
        (void) get_msg ( res_str, 1000, 0 );
        Debug ( " eval_script() got result >>%s<<\n", res_str );

        if ( sscanf ( res_str, "time:%lf", &res ) != 1 )
          {
            Error ( " error in eval result >>%s<<\n", res_str );
            res = -1;
          }
        else
            results[x] = res;

      }

    if ( strcmp ( cmd->prot, "/dev/null" ) != 0 )
      {
        FILE  *p_file;
        char   p_f_name[1024];
        double min, max, sum;

        sprintf ( p_f_name, "%s/%s", Protokoll_dir, cmd->prot );

        if ( ( p_file = fopen ( p_f_name, "a+" ) ) == NULL )
            abbruch ( " can't open file >>%s<<\n", p_f_name );
        min = MIN_VALUE;
        max = 0;
        sum = 0;

        for ( x = 0; x < cmd->n_cpu; x++ )
          {
            sum += results[x];
            if ( results[x] < min )
               min = results[x];
            if ( results[x] > max )
               max = results[x];
          }

           /* format:
                N_cpus  sum  min max  size nr_of_files
                sum min max in files_created/second */
        fprintf ( p_file, "%d  %f %f %f  %s\n",
                  cmd->n_cpu, (float)sum, (float)min, (float)max,
                  cmd->prefix );
        fclose ( p_file );
     }

   return ( 0 );
}

/* ___________________________________________________________________ */


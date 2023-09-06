
#include "disk_io.h"

#include <string.h>

extern   pid_t  My_pid;  /* pid of this process */

/* ________________________________________________________________ */
   struct perf_entry {
	  double     sec;
	  MY_SIZE_T  mb;
	  double     mb_s;
	  double     mb_s_avg;
      };
/* ________________________________________________________________ */

static void mem_initialize ( char *mem, const MY_SIZE_T len )
{
   if ( memset ( mem, 'a', (size_t)len ) != mem )
       abbruch ( " can't initialize memory" );
}

/* ________________________________________________________________ */

static char *init_buffer ( MY_SIZE_T buf_size )
{
   static  char      *Buffer = NULL;
   static  MY_SIZE_T  B_size = 0;

   if ( buf_size <= B_size )
     return ( Buffer );
     
   if ( Buffer != NULL )
     free ( Buffer );

   if ( ( Buffer = (char *)malloc ( (size_t)buf_size ) ) == NULL )
       abbruch ( "can't allocate memory for buffer\n" );

   mem_initialize ( Buffer, buf_size );

   B_size = buf_size;
   return ( Buffer );
} 
  
/* ________________________________________________________________ */

static MY_SIZE_T do_read ( int fd, char *buff, MY_SIZE_T cnt )
{
    MY_SIZE_T ret, have_read;

    have_read = 0;

    do {
         ret = read ( fd, buff, cnt );
         if ( ret > 0 )
           {  cnt       -= ret;
              buff      += ret;
              have_read += ret;
           }
         if ( ret == -1 )
           {
             perror ( "on reading" );
             return ( -1 );
           }
        } while ( cnt > 0 && ret > 0 );

    return ( have_read );             
}

/* ________________________________________________________________ */
static char *make_buff_2_str ( char *buf, MY_SIZE_T cnt )
{
   static char output[80];
   int x;

   for ( x = 0; x < sizeof (output)-1 && x < cnt; x++ )
    {
      if ( isprint ( *buf ) )
         output[x] = *buf;
      else
         output[x] = '.';
      buf++;
    }

   output[x] = '\0';

   return ( output );
}
   
/* ________________________________________________________________ */

int do_write ( int fd, char *buff, MY_SIZE_T cnt )
{
    MY_SIZE_T ret;

    do {
         ret = write ( fd, buff, cnt );
         if ( ret > 0 )
           {  cnt  -= ret;
              buff += ret;
           }
         if ( ret == -1 )
           {
             perror ( "on writing" );
             Error ( " fd: %d  cnt: %ld >>%s<<\n",
                fd, cnt, make_buff_2_str ( buff, cnt ) );
             return ( -1 );
           }
        } while ( cnt > 0 );

    return ( 0 );             
}

/* ________________________________________________________________
 *
 *  path is the absolute filename for the output-file
 *  write (2) take place in chunks of size buf_size bytes
 *  file_size is the size of the output-file in bytes
 *   mode:   == PERF_QUITE   don't report erverything
 *              PERF_ONCE    when finished
 *              PERF_QUITE   nothing
 *   "(pid) filename:path  buf_size:size_of_buffer file_size:size_of_file \
 *    sec:seconds   u_sec:mikro_seconds  b_s:bytes_per_second\n"
 *
 *   on error the field of sec:seconds is returned as sec:-1
 *            the fields u_sec mb_s are set to zero!
 *  _____________________________________________________________________ */ 

int write_a_file ( const char *path, const MY_SIZE_T buf_size, 
				   const MY_SIZE_T file_size, const PERF_OUTPUT mode, const BOOL log )
{
   MY_SIZE_T size_now = 0, b_writes, part_size;
   int fd;
   char  *buff;
   struct timezone tz;
   struct timeval tv_beg, tv_now, tv_open, tv_close, tv_last;
   double mb_s, sec, mb_s_loc, sec_loc;
   MY_SIZE_T  x, msg_count, last_buf, last_size = 0, output_num = 0;


   int i, imax;
   FILE *fp;
   char filename[128];


   struct perf_entry *perf_array = NULL;

   tz.tz_minuteswest = 0;
   tz.tz_dsttime     = 0;

   buff = init_buffer ( buf_size );

   b_writes = file_size / buf_size;
   part_size = file_size % buf_size;

   msg_count = 10 * EIN_MB / buf_size;
   if ( msg_count == 0 )
     msg_count = 1;

   if ( mode == PERF_CONTI && log == B_TRUE )
     {
       output_num=(b_writes/msg_count) + 1;
       perf_array = calloc((size_t) output_num, sizeof(struct perf_entry)); 
     }


   if ( gettimeofday ( &tv_open, &tz ) == -1 )
     Error ( " can't get time by gettimeofday()\n" );

Debug ( "start write tv_open:  %lld sec   %lld usec", 
		 (long long)tv_open.tv_sec, (long long)tv_open.tv_usec );

   if ( (fd = open ( path, O_CREAT|O_TRUNC|O_WRONLY, 0644 )) == -1 )
     { 
        char *err_str;

        err_str = strerror ( errno );
        Error ( " can't create file %s  error: %d  %s\n", path, errno, err_str );
        return ( -1 ); 
     }
    else
     {
       if ( mode == PERF_ONCE )
         {
           Debug ( " in fkt write_a_file() call open() von file >>%s<< ok! "
                   "Warum wurde hier mal was zurueckgemeldet???",
                     path );
           /* print_result ( WR_RES_STR, My_pid, path, 
	          (MY_SIZE_T)buf_size, (MY_SIZE_T)file_size, -1L, 0L, 0.0 );  */
         }
     }

   if ( gettimeofday ( &tv_beg, &tz ) == -1 )
     Error ( " can't get time by gettimeofday()\n" );

   tv_last.tv_sec = tv_beg.tv_sec;
   tv_last.tv_usec = tv_beg.tv_usec;
  
   i=0; 
   for ( x = 0; x < b_writes; x++)
    {
       if ( do_write ( fd, buff, buf_size ) != 0 )
           abbruch ( "writing file %s\n", path );
       
       size_now += buf_size;
       if ( mode == PERF_CONTI && x > 0 && x % msg_count == 0 )
         {
            gettimeofday ( &tv_now, &tz );
            sec = ((double)( tv_now.tv_sec - tv_beg.tv_sec )) + 
                  (( (double)(tv_now.tv_usec - tv_beg.tv_usec ) ) / 1000000 );
            mb_s = size_now / (sec * EIN_MB);

			last_buf = size_now - last_size;

			sec_loc = ((double)( tv_now.tv_sec - tv_last.tv_sec )) +
				  (( (double)(tv_now.tv_usec - tv_last.tv_usec ) ) / 1000000 );
            mb_s_loc = last_buf / (sec_loc * EIN_MB);


            print_info ( 
 "\r  wrote %ldMB     %.2lfMB/s  last buffer of %ldMB  with %.2lfMB/s      ",
                          (long)( size_now / EIN_MB), mb_s,
                          (long)(last_buf/EIN_MB), mb_s_loc); 


            if ( log == B_TRUE ) 
			  {
                if ( i >= output_num )
                  abbruch ( " not enough buffers for performance log" ); 
			    perf_array[i].sec      = sec;
				perf_array[i].mb       = size_now; 
			    perf_array[i].mb_s     = mb_s_loc;
			    perf_array[i].mb_s_avg = mb_s;
                i++;
			  }

            last_size = size_now;
			tv_last.tv_sec = tv_now.tv_sec;
			tv_last.tv_usec = tv_now.tv_usec;

         }
    }

   if ( part_size > 0 )
     {
       if ( do_write ( fd, buff, part_size ) != 0 )
           abbruch ( "writing file %s\n", path );
       size_now += part_size;
     }

   if ( (mode == PERF_CONTI || mode == PERF_SUM) && size_now > 0 )
     {
            gettimeofday ( &tv_now, &tz );
            sec = ((double)( tv_now.tv_sec - tv_beg.tv_sec )) +
                  (( (double)(tv_now.tv_usec - tv_beg.tv_usec ) ) / 1000000 );
            mb_s = size_now / (sec * EIN_MB);

            if ( size_now > EIN_MB )
              print_info ( 
 "\r  wrote %ldMB in %.2lfs     %.2lfMB/s                                  \n",
                  (long)(size_now/EIN_MB), sec, mb_s );
            else
              print_info ( 
 "\r  wrote %ldB in %.2lfs     %.2lfMB/s                                   \n",
                   (long)size_now, sec, mb_s );
      }

   (void)close ( fd );

   if ( gettimeofday ( &tv_close, &tz ) == -1 )
     Error ( " can't get time by gettimeofday()\n" );
   
   sec = ((double)( tv_close.tv_sec - tv_open.tv_sec )) +
                  (( (double)(tv_close.tv_usec - tv_open.tv_usec)) / 1000000 );
   mb_s = file_size / sec;

Debug ( "end write tv_close:  %lld sec   %lld usec   sec: %lf    len: %lld",
	       	(long long)tv_close.tv_sec, (long long)tv_close.tv_usec,
                (double) sec, (long long) file_size );


   if ( mode == PERF_CONTI && log == B_TRUE )
     {
		imax=i; 
	
        sprintf(filename,"%lldMB.%lldk_buf.writelog", 
                (long long) file_size/EIN_MB,(long long) buf_size/1024);	
	    fp = fopen(filename,"w");
        if ( fp == NULL )
		{
		    Error ( " can't create file %s\n", filename );
		    return ( -1 );
	    }	

        (void)fprintf(fp,"#disk_io measurement output\n");
        (void)fprintf(fp,"#time[s]   size[byte]    perf_delta[MB/s]   perf_avg[MB/s]\n");
        
		for( i=0; i<imax; i++)
          {
			(void)fprintf(fp,"%.4lf      %lld     %.2lf      %.2lf\n",perf_array[i].sec,
			        (long long)perf_array[i].mb,perf_array[i].mb_s,perf_array[i].mb_s_avg);
          }
		fclose(fp);
		free(perf_array);
     }

   if ( mode == PERF_ONCE )
     {
       print_result ( WR_RES_STR, My_pid, path,
		       (MY_SIZE_T)buf_size, (MY_SIZE_T)file_size, 
                 (long)(tv_close.tv_sec-tv_open.tv_sec), 
                 (long)(tv_close.tv_usec-tv_open.tv_usec), mb_s);
     }
      
   return ( 0 );
}

/* ________________________________________________________________
 *
 *  path is the absolute filename for the input-file
 *  read(2) take place in chunks of size buf_size bytes
 *   mode:   == PERF_QUITE   don't report erverything
 *              PERF_ONCE    when finished
 *              PERF_QUITE   nothing
 *   "(pid) filename:path  buf_size:size_of_buffer file_size:size_of_file \
 *    sec:seconds   u_sec:mikro_seconds  b_s:bytes_per_second\n"
 *
 *   on error the fild of sec:seconds is returned as sec:-1
 *            the fileds u_sec mb_s are set to zero!
 *  _____________________________________________________________________ */ 

int read_a_file ( const char *path, MY_SIZE_T buf_size, PERF_OUTPUT mode,
				  const BOOL log)
{
   MY_SIZE_T       size_now = 0; 
   MY_SIZE_T     len;
   int        fd;
   char      *buff;
   double     mb_s, sec, mb_s_loc, sec_loc;
   MY_SIZE_T  x, msg_count, last_buf, last_size = 0;
   MY_SIZE_T  output_num = 3000; /* we don't know the file size,
		 this last until 16 GB -- we should do a stat(2) ... */
   struct timezone tz;
   struct timeval tv_beg, tv_now, tv_open, tv_close, tv_last;

   int i, imax;
   FILE *fp;
   char filename[128];

   struct perf_entry *perf_array = NULL;


   tz.tz_minuteswest = 0;
   tz.tz_dsttime     = 0;

   buff = init_buffer ( buf_size );

   msg_count = 10 * EIN_MB / buf_size;
   if ( msg_count == 0 )
     msg_count = 1;

   if ( mode == PERF_CONTI && log == B_TRUE )
     perf_array = calloc((size_t) output_num, sizeof(struct perf_entry)); 


   if ( gettimeofday ( &tv_open, &tz ) == -1 )
     Error ( " can't get time by gettimeofday()\n" );

   if ( (fd = open ( path, O_RDONLY )) == -1 )
     { 
       if ( mode == PERF_ONCE )
         {
           Error ( " can't open file >>%s<<\n", path );
           print_result ( WR_RES_STR, My_pid, path, 
					 (MY_SIZE_T)buf_size, (MY_SIZE_T)size_now, -1L, 0L, 0.0 );
         }
       else
          Error ( " can't read file %s\n", path );
       return ( -1 ); 
     }

   if ( gettimeofday ( &tv_beg, &tz ) == -1 )
     Error ( " can't get time by gettimeofday()\n" );

   tv_last.tv_sec = tv_beg.tv_sec;
   tv_last.tv_usec = tv_beg.tv_usec;

   x = 0;
   i = 0;
   while ( ( len = do_read ( fd, buff, buf_size ) ) > 0 )
    {
       size_now += len;
       if ( mode == PERF_CONTI && x > 0 && x % msg_count == 0 )
         {
            (void)gettimeofday ( &tv_now, &tz );
            sec = ((double)( tv_now.tv_sec - tv_beg.tv_sec )) + 
                  (( (double)(tv_now.tv_usec - tv_beg.tv_usec ) ) / 1000000 );
            mb_s = size_now / (sec * EIN_MB);

            last_buf = size_now - last_size;

            sec_loc = ((double)( tv_now.tv_sec - tv_last.tv_sec )) +
					  (( (double)(tv_now.tv_usec - tv_last.tv_usec ) ) / 1000000 );
            mb_s_loc = last_buf / (sec_loc * EIN_MB);

            print_info ( 
   "\r  read %ldMB     %.2lfMB/s  last buffer of %ldMB  with %.2lfMB/s ",
			              (long)( size_now / EIN_MB), mb_s,
                          (long)(last_buf/EIN_MB), mb_s_loc); 

			if ( log == B_TRUE )
			  {
                if ( i >= output_num )
                  abbruch ( " not enough buffers for performance log" ); 
                perf_array[i].sec      = sec;
                perf_array[i].mb       = size_now; 
                perf_array[i].mb_s     = mb_s_loc;
                perf_array[i].mb_s_avg = mb_s;
                i++;
              }

            last_size = size_now;
            tv_last.tv_sec = tv_now.tv_sec;
            tv_last.tv_usec = tv_now.tv_usec;
         }
       x++;
    }

   if ( (mode == PERF_CONTI || mode == PERF_SUM) && size_now > 0 )
     {
            (void)gettimeofday ( &tv_now, &tz );
            sec = ((double)( tv_now.tv_sec - tv_beg.tv_sec )) +
                  (( (double)(tv_now.tv_usec - tv_beg.tv_usec ) ) / 1000000 );
            mb_s = size_now / (sec * EIN_MB);

            if ( size_now > EIN_MB )
              print_info ( 
  "\r  read %ldMB in %.2lfs     %.2lfMB/s                              \n",
                  (long)(size_now/EIN_MB), sec, mb_s );
            else
              print_info ( 
  "\r  read %ldB in %.2lfs     %.2lfMB/s                               \n",
                   (long)size_now, sec, mb_s );
      }

   (void)close ( fd );

   if ( gettimeofday ( &tv_close, &tz ) == -1 )
     Error ( " can't get time by gettimeofday()\n" );
   
   sec = ((double)( tv_close.tv_sec - tv_open.tv_sec )) +
                  (( (double)(tv_close.tv_usec - tv_open.tv_usec)) / 1000000 );
   mb_s = size_now / sec;


   if ( mode == PERF_CONTI && log == B_TRUE ) 
     {
       imax=i; 
       sprintf(filename,"%lldMB.%lldk_buf.readlog",(long long) (size_now/EIN_MB),
                              (long long) buf_size/1024);       
       fp = fopen(filename,"w");
       if ( fp == NULL )
       {
          Error ( " can't create file %s\n", filename );
          return ( -1 );
       }   
       fprintf(fp,"#disk_io measurement output\n");
       fprintf(fp,"#time[s]   size[byte]    perf_delta[MB/s]   perf_avg[MB/s]\n");
 
 	   for ( i=0; i<imax; i++ )
	     {
           fprintf(fp,"%.4lf      %lld     %.2lf      %.2lf\n", perf_array[i].sec,
				   (long long)perf_array[i].mb, perf_array[i].mb_s, perf_array[i].mb_s_avg);
         }
       fclose(fp);
       free(perf_array);
     }


   if ( mode == PERF_ONCE )
     {
       print_result ( WR_RES_STR, My_pid, path, 
                 (MY_SIZE_T)buf_size, (MY_SIZE_T)size_now, 
                 (long)(tv_close.tv_sec-tv_open.tv_sec), 
                 (long)(tv_close.tv_usec-tv_open.tv_usec), mb_s);
     }
      
   return ( 0 );
}

/* ________________________________________________________________ */


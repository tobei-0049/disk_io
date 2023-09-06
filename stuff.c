/* _____________________________________________________________ 
 *
 *  same functionality as printf, but unbufferd write(2) to stdout
 *  this function allows to redirect stdout (not FILE*stdout)
 *
 * _____________________________________________________________ */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "disk_io.h"

#if (defined(sun) && !defined(SOLARIS_7))
#  include <sys/varargs.h>
#endif

/* _____________________________________________________________ */

int my_printf ( char *str, ... )
{
   char       ausgabe[1024];
   va_list    args;
   int        len;

   va_start ( args, str );
   vsprintf ( ausgabe, str, args );
   va_end   ( args );

   len = strlen ( ausgabe );

   if ( do_write ( 1, ausgabe, len ) != len )
     return ( -1 );
    
   return ( 0 );
}

/* _____________________________________________________________ */

int print_result ( char *str, ... )
{
#if defined(USE_MPI)
   char       ausgabe[1024];
   va_list    args;
   int        len;

   va_start ( args, str );
   vsprintf ( ausgabe, str, args );
   va_end   ( args );

   len = strlen ( ausgabe );

   if ( MPI_Send ( ausgabe, len+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD ) != 0 )
	 abbruch ( "Error in print_result(%d)  MPI_Send", My_id );

   return ( 0 );

#else
#if !defined(VA_DECL_IS_REENTRANT)
   char       ausgabe[1024];
   va_list    args;
   int        len;

   va_start ( args, str );
   vsprintf ( ausgabe, str, args );
   va_end   ( args );

   len = strlen ( ausgabe );

   if ( do_write ( 1, ausgabe, len ) != len )
     return ( -1 );
    
   return ( 0 );
#else
   va_list    args;

   va_start ( args, str );
   my_printf ( str, args );
   va_end   ( args );

   return ( 0 );
#endif  /* VA_DECL_IS_REENTRANT */
#endif
}

/* _____________________________________________________________ */

int print_info ( char *str, ... )
{
#if !defined(VA_DECL_IS_REENTRANT)
   char       ausgabe[1024];
   va_list    args;
   int        len;

   va_start ( args, str );
   vsprintf ( ausgabe, str, args );
   va_end   ( args );

   len = strlen ( ausgabe );

   if ( do_write ( 1, ausgabe, len ) != len )
     return ( -1 );
    
   return ( 0 );
#else
   va_list    args;

   va_start ( args, str );
   my_printf ( str, args );
/*    va_end   ( args );   called in my_printf */

   return ( 0 );
#endif
}


/* _____________________________________________________________ */

void Debug ( char *str, ... )
{
    char ausgabe[1024];
    va_list	args;

    if ( Do_debug == B_FALSE )
      return;


    va_start ( args, str );
    vsprintf ( ausgabe, str, args );
    va_end   ( args );

    fprintf ( stderr, "DEBUG [%3d]: %s\n", My_id, ausgabe );
}

/* _____________________________________________________________ */

void Error ( char *str, ... )
{
	char ausgabe[1024];
	va_list	args;
	
	va_start ( args, str );
	vsprintf ( ausgabe, str, args );
	va_end   ( args );
	
	fprintf ( stderr, "ERROR [%3d]: %s\n", My_id, ausgabe );
}

/* _____________________________________________________________ */

void abbruch ( char *str, ... )
{
	char ausgabe[1024];
	va_list	args;
	
	va_start ( args, str );
	vsprintf ( ausgabe, str, args );
	va_end   ( args );
	
	fprintf ( stderr, "EXIT DUE TO FATAL ERROR [%3d]: %s\n", My_id, ausgabe );
#if defined(USE_MPI)
    MPI_Abort ( MPI_COMM_WORLD, 4711 );
#endif
	
	exit ( 1 );
}

/* _____________________________________________________________ */


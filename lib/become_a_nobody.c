/* $Id$ */
#include <stdio.h>

#include "become_a_nobody.h"
#include "error.h"

void
become_a_nobody( const char *username )
{
   int rval;
   struct passwd *pw;
 
   pw = getpwnam(username);
   if ( pw == NULL )
     {
       err_quit("user '%s' does not exist\n\n", username);
     }

   rval = getuid();
   if ( rval != pw->pw_uid )  
     {
       if ( rval != 0 )
         {
           err_quit("Must be root to setuid to \"%s\"\n\n", username);
         }

       rval = setuid(pw->pw_uid); 
       if ( rval < 0 )
         {
           err_quit("exiting. setuid '%s' error", username);
         }
     }
}

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

#include "become_a_nobody.h"
#include <gm_msg.h>

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

       rval = setgid(pw->pw_gid);
       if ( rval < 0 )
         {
           err_quit("exiting. setgid %d error", (int) pw->pw_gid);
         }

       rval = initgroups(username, pw->pw_gid);
       if ( rval < 0 )
         {
           err_quit("exiting. initgroups '%s', %d error",
             username, (int) pw->pw_gid);
         }

       rval = setuid(pw->pw_uid);
       if ( rval < 0 )
         {
           err_quit("exiting. setuid '%s' error", username);
         }
     }
}

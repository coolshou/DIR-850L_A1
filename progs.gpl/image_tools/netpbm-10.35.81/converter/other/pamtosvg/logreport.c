/* logreport.c: showing information to the user. */

#include "logreport.h"
#include "message.h"

/* Says whether to output detailed progress reports, i.e., all the data
   on the fitting, as we run.  (-log)  */
FILE *log_file = NULL;


void
flush_log_output (void)
{
  if (log_file)
    fflush (log_file);
}


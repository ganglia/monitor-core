#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "msg.h"

#define MAXMSGLEN 1500

int main( void )
{
  XDR x;
  gangliaMessage message;
  int i, rval, encoded;
  char sendbuf[MAXMSGLEN];

  gangliaMetricGroup group;
  gangliaNamedValue_1 values[20];

  /*
   *
struct gangliaMetricGroup_1 {
	int collected;
	u_int tn;
	u_int tmax;
	u_int dmax;
	gangliaSlopeTypes_1 slope;
	gangliaMetricQualities_1 quality;
	struct {
		u_int value_len;
		gangliaNamedValue_1 *value_val;
	} value;
};
typedef struct gangliaMetricGroup_1 gangliaMetricGroup_1;
*/
 
  /* Create a message to send */
  memset(&message, 0, sizeof(gangliaMessage));
  message.format = GANGLIA_METRIC_GROUP;
  message.gangliaMessage_u.group = &group;

  /* Setup the info about the group */
  group.collected = time(NULL);
  group.tn = 1;
  group.tmax = 20;
  group.dmax = 100;
  group.slope = GANGLIA_SLOPE_ZERO_1;
  group.quality = GANGLIA_QUALITY_NORMAL_1;

#define NUM_VALUES 10
  group.data.data_len = NUM_VALUES;
  group.data.data_val = values;
  for(i=0; i<NUM_VALUES; i++)
    {
      values[i].name = strdup("cpu_num");
      values[i].value.type = GANGLIA_VALUE_INT_1;
      values[i].value.gangliaValue_1_u.i = 4;
    }

  /* Create the XDR send stream */
  xdrmem_create(&x, sendbuf, MAXMSGLEN, XDR_ENCODE);

  /* Write the gangliaMessage to the stream */
  xdr_gangliaMessage(&x, &message);

  /* Find out how big the message is encoded */
  encoded = xdr_getpos(&x);
  fprintf(stderr,"CLIENT encoded XDR message size=%d\n", encoded);

  /* Write the buffer to stdout */
  rval = write(1, sendbuf, encoded );
  if(rval != encoded)
    {
      fprintf(stderr,"ERROR! Client unable to send complete XDR message\n");
      exit(1);
    }

  fprintf(stderr,"CLIENT successfully sent XDR message\n");
  return 0;
}

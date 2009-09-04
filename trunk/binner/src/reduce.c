#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/select.h>

#include "reducefunc.h"

#define BUFSIZE 4*1024*1024

int main(int argc, char ** argv)
{
	fd_set readfds, masterfds;
	int i, n, m, nbytes = 0, ninputs;
	char * netbuf;
	int unitsize, toread;

	if (argc != 2)
	{
		fprintf(stderr, "usage: reduce number_of_input_streams\n");
		fprintf(stderr, "%s %s\n", argv[0], argv[1]);
		exit(1);
	}
	
	close(0);

	unitsize = reducefunc_unitsize();

	ninputs = (int)(atof(argv[1]));

	netbuf = malloc(BUFSIZE);
	
	FD_ZERO(&masterfds);

	for (i = 3; i < 3 + ninputs; i++)
		FD_SET(i, &masterfds);
	
	for (m = ninputs ; m > 0; ) 
	{
		/* 
		 * FD_COPY(&masterfds, &readfds);
		 * not portable on all unix flavors
		 */
		memcpy(&readfds, &masterfds, sizeof(fd_set));
		
		if ((n = select(ninputs + 3, &readfds, NULL, NULL, NULL)) < 0)
		{
			perror("select failed in reduce");
			exit(2);
		}

#if REBINDEBUG
		fprintf(stderr, "reduce select returned %d, ninputs = %d \n", n, ninputs);
#endif
		
		for (i = 3; i < 3 + ninputs; i ++)
			if (FD_ISSET(i, &readfds))
			{
				for (n = 0, toread = BUFSIZE; toread > 0; )
				{
					n += read(i, netbuf+n, toread);
					toread = n - (n / unitsize) * unitsize;
				}

#if REBINDEBUG
				fprintf(stder, "reduce read %d bytes from fd: %d\n", n, i);
#endif
				if (n > 0)
				{
					reducefunc(netbuf, n/unitsize);
					nbytes += n;
				}
				else
				{
#if REBINDEBUG
					fprintf(stderr,"reduce input %d is done\n", i - 3);
#endif
					FD_CLR(i, &masterfds);
					m --;
					close(i);
				}
			}
	}

#if REBINDEBUG
	fprintf(stderr,"reduce ninputs = %d \n", ninputs);
#endif
	fprintf(stderr,"reduce wrote %d bytes.\n", nbytes);
	free(netbuf);

	return 0;
}

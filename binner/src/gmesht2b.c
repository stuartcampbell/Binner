/**

   \ingroup rebinner_execs
   
   \file src/gmesht2b.c

   \brief CURRENT executable to convert from ASCII to binary gmesh formats.
  
   gmesht2b < ASCII_gmesh > BINARY_gmesh
  
   \note This executable is designed to act as a filter.
   
   $Id$

 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


int main(int argc, char ** argv)
{
	int nfacets, i, n, sliceid;
	double * vdata;
	double emin, emax, hitcnt, hiterr, threshold;
	char * buf;
	int lastchar[50], c;

	buf = malloc(1024*8);
	vdata = malloc(1024 * sizeof(double));

	threshold = 1e-16;

	if (argc == 3)
	{
		if (strcmp(argv[1],"-t") == 0)
			threshold = atof(argv[2]);
		else
		{
			fprintf(stderr, "usage: %s [-t threshhold] \n", argv[0]);
			exit(1);
		}
	}
	else if (argc != 1)
	{
		fprintf(stderr, "usage: %s [-t threshhold] \n", argv[0]);
		exit(1);
	}

	for (nfacets = 0; fgets(buf, 1024*8, stdin) != NULL; ) 
    {
		for (i = 0, c = 0; buf[i] != '\0'; i ++)
			if (isspace(buf[i]))
			{
				buf[i] = '\0';
				lastchar[c] = i;
				c ++;
			}
		
		if (c == 29) 
			nfacets ++;
		else
			continue;

		hitcnt = atof(buf + lastchar[2] + 1);
		if (hitcnt < threshold) continue;

		c = 0;
		sliceid = atoi(buf+c); c = lastchar[0] + 1;
		emin = atof(buf + c);  c = lastchar[1] + 1;
		emax = atof(buf + c);  c = lastchar[2] + 1;
		hitcnt = atof(buf +c); c = lastchar[3] + 1;
		hiterr = atof(buf +c); c = lastchar[4] + 1;

		for (n = 0; n < 8 * 3; n ++)
		{
			vdata[n] = atof(buf + c);
			c = lastchar[n + 5] + 1;
		}

		fwrite(&sliceid, sizeof(int), 1, stdout);
		fwrite(&emin, sizeof(double), 1, stdout);
		fwrite(&emax, sizeof(double), 1, stdout);
		fwrite(&hitcnt, sizeof(double), 1, stdout);
		fwrite(&hiterr, sizeof(double), 1, stdout);
		fwrite(vdata, sizeof(double), 8*3, stdout);
	}

	free(vdata);
	free(buf);
	
	/* fprintf(stderr, "nfacets = %d\n", nfacets); */
	
	if (nfacets > 0) 
		return 0; /* successful */
	else
		return 1; /* failure */

}

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "binner.h"
#include "vcbutils.h"
#include "vcblinalg.h"
#include "voxelize.h"
#include "volume.h"


void bounding_box(int n, double * bound, double * v)
{
	double  fimin,fjmin,fkmin,fimax,fjmax,fkmax;
	int k;
	double * vp;

	fimin = fjmin = fkmin = 1e6;
	fimax = fjmax = fkmax = -1e6;

	for (k = 0, vp = v; k < n; vp += 3, k++)
	{
		if (vp[0]<fimin)  fimin=vp[0];
		if (vp[0]>fimax)  fimax=vp[0];
		if (vp[1]<fjmin)  fjmin=vp[1];
		if (vp[1]>fjmax)  fjmax=vp[1];
		if (vp[2]<fkmin)  fkmin=vp[2];
		if (vp[2]>fkmax)  fkmax=vp[2];
	}
	
	bound[0] = fimin;
	bound[1] = fimax;
	bound[2] = fjmin;
	bound[3] = fjmax;
	bound[4] = fkmin;
	bound[5] = fkmax;

}

void flood_wvol(unsigned int * wvol, int * xyzsize)
{
	int notdone, i, j, k, id;
	int xy = xyzsize[2]*xyzsize[1];
	int xyz = xy*xyzsize[0];
	int xdim = xyzsize[2];
	
	unsigned int * vol;
	unsigned int seed = 0xffffffff;

	vol = (unsigned int *) wvol;
	
	
	vol[0] = seed; /* seed flooding at the origin */

	for (notdone = 1; notdone > 0; ) {
		//printf("flooding ...");
		fflush(stdout);
		notdone = 0;
		for(k = 0; k < xyz; k += xy)
			for(j = 0; j < xy; j += xdim)
				for(i=0, id = k+j; i < xdim; i++, id++)
					if (vol[id] == 0)
						if ((i == 0) || (j == 0) || (k == 0) ||
							(vol[id-1] == seed) || 
							(vol[id-xdim] == seed) || 
							(vol[id-xy] == seed))
						   vol[id] = seed;

		for(k = xyz-xy; k >= 0; k -= xy)
			for(j = xy-xdim; j >= 0; j -= xdim)
				for(i = xdim-1, id = k+j+i; i >= 0; i--, id--) {

					if (vol[id] == 0)
						if ((i ==  xdim-1) || (j == xy-xdim) || (k == xyz-xy) ||
							(vol[id+1]    == seed) ||
						    (vol[id+xdim] == seed) ||
						    (vol[id+xy]   == seed))
						   vol[id] = seed;

					/* check and see if we are done. should be by here */
					if	(vol[id] == seed)
						if (((i  <  xdim-1) && (vol[id+1] == 0)) ||
						    ((j < xy-xdim) && (vol[id+xdim] == 0)) ||
						    ((k <  xyz-xy) && (vol[id+xy] == 0)))
						   notdone = 1;
			}
		//printf(" pass\n");
	}
	
	for (i = 0, j = 0, k = 0; i < xyz; i ++) {
	  if (vol[i] == 0xffffffff) j ++;
	  if (vol[i] == 0) k ++;
	}
	
	printf("nvoxel %d, out/in/sur: %d/%d/%d\n", xyz, j, k, xyz-j-k);
}

double count_volume(double * vol, int * xyzsize)
{
	int i; 
	int xyz = xyzsize[0]*xyzsize[1]*xyzsize[2];
	double volume;
	
	for (i = 0, volume = 0.0; i < xyz; volume += vol[i], i ++);
	
	return volume;
}

/*
 * voxels == NULL: bin_quad3d is only to return the computed volume
 * otherwise:      bin_quad3d should write out the discrete volume
 *                 to voxels. the memory has to be enough for holding
 *                 xyzsize[0]*xyzsize[1]*xyzsize[2] double values
 */
double bin_para3d_150(	int		nfacets, 
						int   * nverts,
						double *v, /* the vertices */
						int *	orig, 
						int *	xyzsize,
						double  ccs, /* cubic cell size */ 
						double *voxels) 
{
	clock_t time1, time2, time3, time4;

	/* return value: is the total volume in the subdivided grid */
	double cs, voxel_volume, total_volume, inside_vol, surface_vol;
	double * vp;
	double bounds[6];
	double wv[6*4*3];
	int    wnfacets, wnverts[6];
	int    *face_starts;
	//char   fname[100];

	int i, j, k, nvoxel, id, vid;
	unsigned int temp, pid;
	int lb[3], ub[3], ori[3], coord[3], totalverts; 
	unsigned int *wvol; /* working volume */
	int wsize[3];

	unsigned int base = 1024;

time1 = clock();

	face_starts = malloc(nfacets * sizeof(int));
	/* find out the bounding box */
	for (i = 0, totalverts = 0; i < nfacets; totalverts += nverts[i], i ++)
		face_starts[i] = totalverts;

	bounding_box(totalverts, bounds, v);

	/* ccs: cubic cell size */
	/* assume uniform cell size */
	cs = ccs/2;  /*half of cell size */

	/* now let's pad 2 cells on each end of the 3 dimensions */
	bounds[0] -= ccs*2;
	bounds[1] += ccs*3;
	bounds[2] -= ccs*2;
	bounds[3] += ccs*3;
	bounds[4] -= ccs*2;
	bounds[5] += ccs*3;
	
	/* make sure bounds end on integar intervals of ccs */
	lb[0]     = (int)floor(bounds[0]/ccs);
	bounds[0] = lb[0] * ccs;
	lb[1]     = (int)floor(bounds[2]/ccs);
	bounds[2] = lb[1] * ccs;
	lb[2]     = (int)floor(bounds[4]/ccs);
	bounds[4] = lb[2] * ccs;

	ub[0]     = (int)ceil(bounds[1]/ccs);
	bounds[1] = ub[0] * ccs;
	ub[1]     = (int)ceil(bounds[3]/ccs);
	bounds[3] = ub[1] * ccs;	
	ub[2]     = (int)ceil(bounds[5]/ccs);
	bounds[5] = ub[2] * ccs;			

	/* knowing the proper bounds, wvol should be of wsize */
	wsize[0] = ub[0] - lb[0];
	wsize[1] = ub[1] - lb[1];
	wsize[2] = ub[2] - lb[2];

	nvoxel = wsize[0]*wsize[1]*wsize[2];
	//printf("wsize: %d %d %d\n",wsize[0], wsize[1], wsize[2]);

	wvol = malloc(nvoxel * sizeof(unsigned int));
	if (wvol == NULL) {
		perror("bin_para3d_150 wvol allocation error");
		exit(1);
	}

/*
	for (i = 0; i < nvoxel; wvol[i] = 100, i ++);
	
	for (i = 0; i < wsize[0]; i ++)
		for (j = 0; j < wsize[1]; j ++)
			for (k = 0; k < wsize[2]; k ++) 
				wvol[(i*wsize[1]+j)*wsize[2] +k] = k;
	
	ori[0] = ori[1] = ori[2] = 0;
*/
	//vcbGenBinm("100.bin", VCB_UNSIGNEDINT, 3, ori, wsize, 1, wvol);

	memset(wvol, 0, nvoxel * sizeof(unsigned int));

	voxel_volume = ccs * ccs * ccs;

	/* (0,0,0) in wvol should appear on ori[0-3] in voxels */
	ori[0] = lb[0] - orig[0];
	ori[1] = lb[1] - orig[1];
	ori[2] = lb[2] - orig[2];
	//printf("ori: %d %d %d\n",ori[0], ori[1], ori[2]);

	for (i = 0, vp= v; i < nfacets; vp += nverts[i]*3, i ++) {
		/*
		 * facet[i] starts at v[i*(3*4)] and lasts 12 values
		 *          i.e. 4 vertices of xyz coordinates
		 */
		//printf("facet #%d\n", i);
		voxelize_poly(nverts[i],
					  vp,
					  wvol,
					  lb,
					  wsize,
					  ccs,
					  base,
					  i+1);
		//sprintf(fname,"1%d.bin", i);
		//vcbGenBinm(fname, VCB_UNSIGNEDINT, 3, ori, wsize, 1, wvol);

	}

time2 = clock();

	for (i = 0, j = 0, k = 0; i < nvoxel; i ++) {
	  if (wvol[i] > 0) j ++;
	  if (wvol[i] == 0) k ++;
	}
	
	//printf("nvoxel %d, out+in/sur: %d/%d\n", nvoxel, k, j);

	//vcbGenBinm("150.bin", VCB_UNSIGNEDINT, 3, ori, wsize, 1, wvol);

	flood_wvol(wvol, wsize);

	//vcbGenBinm("200.bin", VCB_UNSIGNEDINT, 3, ori, wsize, 1, wvol);

time3 = clock();

	//printf("computing volume, orig: %d %d %d -- wvol bounded by (%d,%d,%d) - (%d,%d,%d) ... \n", orig[0], orig[1], orig[2], lb[0], lb[1], lb[2], ub[0], ub[1], ub[2]);

	/* now let's figure out the volumes */
	total_volume = inside_vol = surface_vol = 0.0;
	nvoxel = 0;
	for (i = 0; i < xyzsize[0]; i ++)
		for (j = 0; j < xyzsize[1]; j ++)
			for (k = 0; k < xyzsize[2]; k ++) 
			{
				/*id = (i * wsize[1] + j) * wsize[2] + k;*/
				vid = (i*xyzsize[1] + j)*xyzsize[2] + k;
				id = ((i-ori[0])*wsize[1] + (j-ori[1]))*wsize[2] + (k-ori[2]);

				temp = wvol[id];
				
				if (temp == 0xffffffff) /* you are outside */
					continue;
				
				if (temp == 0) /* you are inside */
				{	
					voxels[vid] = voxel_volume;
					total_volume += voxel_volume;
					inside_vol += voxel_volume;
					nvoxel ++;
					continue;
				}
				
				/* if you are still here, you are on an edge */
				/*printf("id (%d,%d,%d): temp: %d ",i, j, k, temp);
				*/
				for (wnfacets = 0, vp = wv; temp > 0; temp /= base) {
				    assert(temp % base > 0);
					pid = temp % base - 1;
					wnverts[wnfacets] = nverts[pid];
					memcpy(vp, &v[face_starts[pid]*3], nverts[pid]*3*sizeof(double));
					
					vp += nverts[pid]*3;
					wnfacets ++;
					/*printf("# %d %d ", pid, nverts[pid]);*/
				}

				coord[0] = i - orig[0];
				coord[1] = j - orig[1];
				coord[2] = k - orig[2];
				/*
				printf(" coord (%d, %d, %d) ",coord[0], coord[1], coord[2]);
				*/
				voxels[vid] = partialvoxel_volume(wnfacets, wnverts, wv, coord, ccs);
				
				total_volume += voxels[vid];
				surface_vol += voxels[vid];
				/*printf(" volume = %lf\n", voxels[vid]);*/
			}

time4 = clock();

	free(wvol);
	free(face_starts);
	//printf("volsz: %d %d %d. ", xyzsize[0], xyzsize[1], xyzsize[2]);
	
	printf("volume(total:in:surf) = %lf:%lf:%lf.\n", total_volume, inside_vol, surface_vol);
	printf("time(voxelize:flood:rebin) = %.3f:%.3f:%.3f sec\n",
			(float)(time2-time1)/CLOCKS_PER_SEC,
			(float)(time3-time2)/CLOCKS_PER_SEC,
			(float)(time4-time3)/CLOCKS_PER_SEC);

	return total_volume;
}

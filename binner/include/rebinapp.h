#ifndef _REBINAPP_
#define _REBINAPP_

#ifdef __cplusplus
extern "C" {
#endif

void	scale_vertices(int n, double * vdata,double xscale, double yscale, double zscale);
double	fv_bounds(double * askbounds, double * spacing, int * orig, int * xyzsize);
double	padded_bounds(double * bounds, int res, int * orig, int * xyzsize);

double	rebin_byslice(int npara,
                     int   * nverts,
					 double *vdata, /* the vertices */
					 int    *sliceid,
					 double *hitcnt,
					 double *hiterr,
					 int *	orig, 
					 int *	xyzsize,
					 double  cellsize, /* assume uniform cell size, hence cubic cells */
					 double *spacing,
					 double *voxels,
					 char * outputpath);

double rebin_gmesh(int npara, 			
					int   * nverts,
			double *vdata, /* the vertices */
			int    *sliceid,
			double *hitcnt,
			double *hiterr,
			int *	orig, 
			int *	xyzsize,
			double  cellsize, /* assume uniform cell size, hence cubic cells */
			double *spacing,
			double *voxels,
			double emin, double emax);

void	output_askinginfo(double * askbounds, int * xyzsize, double * spacing);
void	output_askinginfo_short(int * xyzsize);
void	output_actualinfo(double * bounds);
void	output_prerebininfo(int * orig, int * xyzsize, double cellsize);
void	output_postrebininfo(float rebintime, int npara, double totalvolume, int nvoxel);
void	output_gmesh_formaterr();

#ifdef __cplusplus
}  /* extern C */
#endif

#endif
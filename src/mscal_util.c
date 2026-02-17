/**
         mscal_util.c
**/

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
*/

#include "ucvm_model_dtypes.h"
#include "mscal.h"
#include "um_netcdf.h"

#include "mscal_util.h"

/**** for mscal_dataset_t ****/
mscal_dataset_t *make_a_mscal_dataset(char *datadir, char *datafile, int tooBig) {

    char filepath[256];
    size_t nelems= 0;
    nc_type vtype;

    mscal_dataset_t *data=(mscal_dataset_t *)malloc(sizeof(mscal_dataset_t));

    sprintf(filepath, "%s/%s", datadir, datafile);
    if(mscal_ucvm_debug) fprintf(stderrfp," data file ..%s\n", filepath);

/* setup ncid */
    data->ncid=open_nc(filepath);
  
/* setup nx/ny/nz and void ptrs */
    data->longitudes=(float *) get_nc_buffer(data->ncid, "longitude", filepath, &vtype, &nelems, 1);
    data->nx=nelems;

    data->latitudes=(float *) get_nc_buffer(data->ncid, "latitude", filepath, &vtype, &nelems, 1);
    data->ny=nelems;

    data->depths=(float *) get_nc_buffer(data->ncid, "depth", filepath, &vtype, &nelems, 1);
    data->nz=nelems;

/* Get variable ID by name */
    data->vp_varid=get_nc_varid(data->ncid,"vp",filepath);
    data->vs_varid=get_nc_varid(data->ncid,"vs",filepath);
    data->rho_varid=get_nc_varid(data->ncid,"rho",filepath);

    data->layer_cache_cnt=0;
    data->col_cache_cnt=0;

    data->in_memory =0;

    if(!tooBig) {
/* load all vp/vs/rho data in memory */
        int total= data->nx * data->ny * data->nz;

        data->vp_buffer = (float *)malloc(total * sizeof(float));
        data->vs_buffer = (float *)malloc(total * sizeof(float));
        data->rho_buffer = (float *)malloc(total * sizeof(float));

        data->vp_buffer=get_nc_buffer(data->ncid, "vp", filepath, &vtype, &nelems, 3);
        data->vs_buffer=get_nc_buffer(data->ncid, "vs", filepath, &vtype, &nelems, 3);
        data->rho_buffer=get_nc_buffer(data->ncid, "rho", filepath, &vtype, &nelems, 3);

	data->elems=total;
        data->in_memory=1;

        } else {
	  data->elems=0;
	  data->vp_buffer=NULL;
	  data->vs_buffer=NULL;
	  data->rho_buffer=NULL;
    }

    return data;
}


int free_mscal_dataset(mscal_dataset_t *data) {
    free(data->depths);
    free(data->latitudes);
    free(data->longitudes);
    if(data->vp_buffer != NULL) free(data->vp_buffer);
    if(data->vs_buffer != NULL) free(data->vs_buffer);
    if(data->rho_buffer != NULL) free(data->rho_buffer);
    nc_close(data->ncid);

    //free the caches
    for(int i=0; i< data->layer_cache_cnt; i++) {
      free_a_cache_layer(data->layer_cache[i]);
    }
    for(int i=0; i< data->col_cache_cnt; i++) {
      free_a_cache_col(data->col_cache[i]);
    }

    free(data);
    return SUCCESS;
}

/**** for mscal_cache_col_t ****/
mscal_cache_col_t *_add_a_cache_col(mscal_dataset_t *dataset, int target_lat_idx, int target_lon_idx) {
 
   int nz=dataset->nz;

   mscal_cache_col_t *col= (mscal_cache_col_t *) malloc(sizeof(mscal_cache_col_t));

   col->cache_col_lat_idx=target_lat_idx;
   col->cache_col_lon_idx=target_lon_idx;

   // alloc space first 
   col->col_vp_buffer=(float *) malloc(nz * sizeof(float));
   col->col_vs_buffer=(float *) malloc(nz * sizeof(float));
   col->col_rho_buffer=(float *) malloc(nz * sizeof(float));

   cache_depth_col_float(dataset->ncid, dataset->vp_varid,
                        nz, target_lat_idx, target_lon_idx, col->col_vp_buffer);
   cache_depth_col_float(dataset->ncid, dataset->vs_varid,
                        nz, target_lat_idx, target_lon_idx, col->col_vs_buffer);
   cache_depth_col_float(dataset->ncid, dataset->rho_varid,
                        nz, target_lat_idx, target_lon_idx, col->col_rho_buffer);
   return col;
}

mscal_cache_col_t *find_a_cache_col(mscal_dataset_t *dataset, int target_lat_idx, int target_lon_idx) {
   int max= dataset->col_cache_cnt; 
   mscal_cache_col_t *col;
   for(int i=0; i< max; i++) {
     col=dataset->col_cache[i];
     if((col->cache_col_lat_idx == target_lat_idx) &&
		      (col->cache_col_lon_idx == target_lon_idx) ) {
        // found it
        return col;
     }
   }
   // load it from the netcdf file
   col=_add_a_cache_col(dataset, target_lat_idx, target_lon_idx);
    
   // find a space to put in (in case it is full)
   int idx= ((max+1) % MSCAL_CACHE_COL_MAX); // next index to use
   if(dataset->col_cache[idx]!= NULL) {
       free_a_cache_col(dataset->col_cache[idx]);
       dataset->col_cache[idx]=col; 
       dataset->col_cache_cnt=max; // no change
       } else {
          dataset->col_cache[max]=col; 
          dataset->col_cache_cnt=max+1; 
   }
   return col;     
}

void free_a_cache_col(mscal_cache_col_t *col) {

   // free buffer first 
   free(col->col_vp_buffer);
   free(col->col_vs_buffer);
   free(col->col_rho_buffer);

   free(col);
}

/**** for mscal_cache_layer_t ****/
mscal_cache_layer_t *_add_a_cache_layer(mscal_dataset_t *dataset, int target_dep_idx) {

    int nx=dataset->nx;
    int ny=dataset->ny;

    mscal_cache_layer_t *layer= (mscal_cache_layer_t *) malloc(sizeof(mscal_cache_layer_t));

    layer->cache_layer_dep_idx=target_dep_idx;

    layer->layer_vp_buffer = (float * )malloc((nx*ny) * sizeof(float));
    layer->layer_vs_buffer = (float * )malloc((nx*ny) * sizeof(float));
    layer->layer_rho_buffer = (float * )malloc((nx*ny) * sizeof(float));

    cache_latlon_layer_float(dataset->ncid, dataset->vp_varid,
                      target_dep_idx, ny, nx, layer->layer_vp_buffer);
    cache_latlon_layer_float(dataset->ncid, dataset->vs_varid,
                      target_dep_idx, ny, nx, layer->layer_vs_buffer);
    cache_latlon_layer_float(dataset->ncid, dataset->rho_varid,
                      target_dep_idx, ny, nx, layer->layer_rho_buffer);
    return layer;
}

mscal_cache_layer_t *find_a_cache_layer(mscal_dataset_t *dataset, int target_dep_idx) {

   int max= dataset->layer_cache_cnt;

   mscal_cache_layer_t *layer;
   for(int i=0; i< max; i++) {
     layer=dataset->layer_cache[i];
     if((layer->cache_layer_dep_idx == target_dep_idx) ) {
        // found it
        return layer;
     }
   }
   // load it from the netcdf file
   layer=_add_a_cache_layer(dataset, target_dep_idx);

   // find a space to put in (in case it is full)
   int idx= ((max+1) % MSCAL_CACHE_LAYER_MAX); // next index to use
   if(dataset->layer_cache[idx]!= NULL) {
       free_a_cache_layer(dataset->layer_cache[idx]);
       dataset->layer_cache[idx]=layer;
       dataset->layer_cache_cnt=max; // no change
       } else {
          dataset->layer_cache[max]=layer;
          dataset->layer_cache_cnt=max+1; 
   }
   return layer;     
}

void free_a_cache_layer(mscal_cache_layer_t *layer) {

   free(layer->layer_vp_buffer);
   free(layer->layer_vs_buffer);
   free(layer->layer_rho_buffer);

   free(layer);
}


/**
         muscal_util.c
**/

#include <string.h>

#include "ucvm_model_dtypes.h"
#include "muscal.h"
#include "um_netcdf.h"

#include "muscal_util.h"

/**** for muscal_dataset_t ****/
muscal_dataset_t *make_a_muscal_dataset(char *datadir, char *datafile, int tooBig, int useBinary) {
    char filepath[256];
    size_t nelems= 0;
    nc_type vtype;

    muscal_dataset_t *data=(muscal_dataset_t *)malloc(sizeof(muscal_dataset_t));

    sprintf(filepath, "%s/%s", datadir, datafile);
    if(muscal_ucvm_debug) fprintf(stderrfp," data file ..%s\n", filepath);

/* setup ncid */
    data->ncid=open_nc(filepath);
  
/* setup nx/ny/nz and void ptrs */
    data->longitudes=(float *) get_nc_float_buffer(data->ncid, "longitude", filepath, &vtype, &nelems, 1);
    data->nx=nelems;
    if(muscal_ucvm_debug) {
        fprintf(stderrfp, "  Longitudes: %d\n", nelems);
        for(int i=0;i<nelems; i++) {
            fprintf(stderrfp, "%d  %f\n", i, data->longitudes[i]);
       	}
    }

    data->latitudes=(float *) get_nc_float_buffer(data->ncid, "latitude", filepath, &vtype, &nelems, 1);
    data->ny=nelems;
    if(muscal_ucvm_debug) {
        fprintf(stderrfp, "  Latitude: %d\n", nelems);
        for(int i=0;i<nelems; i++) {
            fprintf(stderrfp, "%d  %f\n", i, data->latitudes[i]);
       	}
    }

    data->depths=(float *) get_nc_float_buffer(data->ncid, "depth", filepath, &vtype, &nelems, 1);
    data->nz=nelems;
    if(muscal_ucvm_debug) {
        fprintf(stderrfp, "  Depths: %d\n", nelems);
        for(int i=0;i<nelems; i++) {
            fprintf(stderrfp, "%d  %f\n", i, data->depths[i]);
       	}
    }

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

	if(!useBinary) {
            data->vp_buffer=get_nc_float_buffer(data->ncid, "vp", filepath, &vtype, &nelems, 3);
            data->vs_buffer=get_nc_float_buffer(data->ncid, "vs", filepath, &vtype, &nelems, 3);
            data->rho_buffer=get_nc_float_buffer(data->ncid, "rho", filepath, &vtype, &nelems, 3);
	    } else {
                data->vp_buffer = get_binary_float_buffer(datadir, "vp.dat", total);
                data->vs_buffer = get_binary_float_buffer(datadir, "vs.dat", total);
                data->rho_buffer = get_binary_float_buffer(datadir, "rho.dat", total);
        }
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



int free_muscal_dataset(muscal_dataset_t *data) {
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

/**** straight or trilinear/bilinear ****/
int get_one_property(muscal_dataset_t *dataset, muscal_pt_info_t *pt, muscal_properties_t *data) {

    int nx=dataset->nx;
    int ny=dataset->ny;
    int nz=dataset->nz;

    float *vp_buffer=dataset->vp_buffer;
    float *vs_buffer=dataset->vs_buffer;
    float *rho_buffer=dataset->rho_buffer;

    int x_idx=pt->lon_idx;
    int y_idx=pt->lat_idx;
    int z_idx=pt->dep_idx;

    int offset= (z_idx)*(ny * nx)+(y_idx)*(nx)+x_idx;

    if(muscal_ucvm_debug) { fprintf(stderrfp,"\nTarget offset %d : idx lon/lat/dep = %d/%d/%d\n", offset,x_idx, y_idx, z_idx); }

    data->vp=vp_buffer[offset];
    data->vs=vs_buffer[offset];
    data->rho=rho_buffer[offset];
    return offset;
}

void get_interp_property(muscal_dataset_t *dataset, muscal_pt_info_t *pt, muscal_properties_t *data) {
	// XXX

	return;
}


/**** for muscal_cache_col_t ****/
muscal_cache_col_t *_add_a_cache_col(muscal_dataset_t *dataset, int target_lat_idx, int target_lon_idx) {
 
   int nz=dataset->nz;

   muscal_cache_col_t *col= (muscal_cache_col_t *) malloc(sizeof(muscal_cache_col_t));

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

muscal_cache_col_t *find_a_cache_col(muscal_dataset_t *dataset, int target_lat_idx, int target_lon_idx) {

   int cnt= dataset->col_cache_cnt; 
   muscal_cache_col_t *col;

   for(int i=0; i< cnt; i++) {
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
   if( cnt < MUSCAL_CACHE_COL_MAX) {
       dataset->col_cache[cnt]=col;
       dataset->col_cache_cnt=cnt+1;
       } else {
// else has to free one out
         int use_idx=(cnt+1) % MUSCAL_CACHE_COL_MAX;
         free_a_cache_col(dataset->col_cache[use_idx]);
         dataset->col_cache[use_idx]=col;

   }

   return col;     
}

void free_a_cache_col(muscal_cache_col_t *col) {

   // free buffer first 
   free(col->col_vp_buffer);
   free(col->col_vs_buffer);
   free(col->col_rho_buffer);

   free(col);
}

/**** for muscal_cache_layer_t ****/
muscal_cache_layer_t *_add_a_cache_layer(muscal_dataset_t *dataset, int target_dep_idx) {

    int nx=dataset->nx;
    int ny=dataset->ny;

    if(muscal_ucvm_debug) { fprintf(stderrfp, "  Loading a new layer: %zu\n", target_dep_idx); }
    muscal_cache_layer_t *layer= (muscal_cache_layer_t *) malloc(sizeof(muscal_cache_layer_t));

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

muscal_cache_layer_t *find_a_cache_layer(muscal_dataset_t *dataset, int target_dep_idx) {

   int cnt= dataset->layer_cache_cnt;
   muscal_cache_layer_t *layer;

   for(int i=0; i< cnt; i++) {
     layer=dataset->layer_cache[i];
     if((layer->cache_layer_dep_idx == target_dep_idx) ) {
        // found it
        return layer;
     }
   }
   // load it from the netcdf file
   layer=_add_a_cache_layer(dataset, target_dep_idx);

   // find a space to put in (in case it is full)
   if( cnt < MUSCAL_CACHE_LAYER_MAX) {
       dataset->layer_cache[cnt]=layer;
       dataset->layer_cache_cnt=cnt+1;
       } else {
// else has to free one out
         int use_idx=(cnt+1) % MUSCAL_CACHE_LAYER_MAX;
         free_a_cache_layer(dataset->layer_cache[use_idx]);
         dataset->layer_cache[use_idx]=layer;
   }
   return layer;     
}

void free_a_cache_layer(muscal_cache_layer_t *layer) {

   free(layer->layer_vp_buffer);
   free(layer->layer_vs_buffer);
   free(layer->layer_rho_buffer);

   free(layer);
}

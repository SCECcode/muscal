/**
 * @file muscal_util.h
 *
**/

#ifndef MUSCAL_UTIL_H
#define MUSCAL_UTIL_H

#define MUSCAL_DATASET_MAX 10

#define MUSCAL_CACHE_LAYER_MAX 20
#define MUSCAL_CACHE_COL_MAX 10

typedef struct bucket_t{
    size_t value;   // the unique index
    size_t count;   // frequency
} bucket_t;

typedef struct muscal_cache_layer_t {
     int cache_layer_dep_idx;
     float *layer_vp_buffer; // nx * ny
     float *layer_vs_buffer;
     float *layer_rho_buffer;
} muscal_cache_layer_t;


typedef struct muscal_cache_col_t {
     int cache_col_lat_idx;
     int cache_col_lon_idx;
     float *col_vp_buffer; // nz
     float *col_vs_buffer;
     float *col_rho_buffer;
} muscal_cache_col_t;


/** The MUSCAL a dataset's working structure. */
typedef struct muscal_dataset_t {
	/** tracking netcdf id **/
        int ncid;

	/** Number of x(lon) points */
	int nx;
	/** Number of y(lat) points */
	int ny;
	/** Number of z(dep) points */
	int nz;

	/** list of longitudes **/
	float *longitudes;
	/** list of latitudes **/
	float *latitudes;
	/** list of depths **/
	float *depths;

	int vp_varid;
	int vs_varid;
	int rho_varid;

	int elems;
	float *vp_buffer;
	float *vs_buffer;
	float *rho_buffer; 

	// track how many cached layers
        int layer_cache_cnt;
        muscal_cache_layer_t *layer_cache[MUSCAL_CACHE_LAYER_MAX];

/* a cache of previous column from cache_depth_col_float call */
	int col_cache_cnt;
        muscal_cache_col_t *col_cache[MUSCAL_CACHE_COL_MAX];

/* flag to show if data i read in memory */
        int in_memory;

} muscal_dataset_t;


/* utilitie functions */
muscal_dataset_t *make_a_muscal_dataset(char *datadir, char *datafile, int tooBig, int useBinary);
int free_muscal_dataset(muscal_dataset_t *data);
muscal_cache_col_t *find_a_cache_col(muscal_dataset_t *dataset, int target_lat_idx, int target_lon_idx);
void free_a_cache_col(muscal_cache_col_t *col);
muscal_cache_layer_t *find_a_cache_layer(muscal_dataset_t *dataset, int target_dep_idx);
void free_a_cache_layer(muscal_cache_layer_t *layer);

int bucket_an_array(size_t *idx_arr, size_t n);

#endif


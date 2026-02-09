/**
 * @file mscal.c
 * @brief Main file for mscal model
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 */

#include <limits.h>
#include "ucvm_model_dtypes.h"
#include "mscal.h"
#include "um_netcdf.h"
#include "cJSON.h"

int mscal_ucvm_debug=0;
FILE *stderrfp=NULL;

int TooBig=1;

/** The config of the model */
char *mscal_config_string=NULL;
int mscal_config_sz=0;

// Constants
/** The version of the model. */
const char *mscal_version_string = "mscal";

// Variables
/** Set to 1 when the model is ready for query. */
int mscal_is_initialized = 0;

char mscal_data_directory[128];

/** Configuration parameters. */
mscal_configuration_t *mscal_configuration;
/** Holds pointers to the velocity model data OR indicates it can be read from file. */
mscal_model_t *mscal_velocity_model;

/**
 * Initializes the mscal plugin model within the UCVM framework. In order to initialize
 * the model, we must provide the UCVM install path and optionally a place in memory
 * where the model already exists.
 *
 * @param dir The directory in which UCVM has been installed.
 * @param label A unique identifier for the velocity model.
 * @return Success or failure, if initialization was successful.
 */
int mscal_init(const char *dir, const char *label) {
    int tempVal = 0;
    char configbuf[512];
    double north_height_m = 0, east_width_m = 0, rotation_angle = 0;

    if(mscal_ucvm_debug) {
      stderrfp = fopen("mscal_debug.log", "w+");
      fprintf(stderrfp," ===== START ===== \n");
    }

    // Initialize variables.
    mscal_configuration = calloc(1, sizeof(mscal_configuration_t));
    mscal_velocity_model = calloc(1, sizeof(mscal_model_t));

    mscal_config_string = calloc(MSCAL_CONFIG_MAX, sizeof(char));
    mscal_config_string[0]='\0';
    mscal_config_sz=0;

    // Configuration file location.
    sprintf(configbuf, "%s/model/%s/data/config", dir, label);

    // Read the configuration file.
    if (mscal_read_configuration(configbuf, mscal_configuration) != UCVM_MODEL_CODE_SUCCESS) {
           // Try another, when is running in standalone mode..
       sprintf(configbuf, "%s/data/config", dir);
       if (mscal_read_configuration(configbuf, mscal_configuration) != UCVM_MODEL_CODE_SUCCESS) {
           mscal_print_error("No configuration file was found to read from.");
           return UCVM_MODEL_CODE_ERROR;
           } else {
           // Set up the data directory.
               sprintf(mscal_data_directory, "%s/data/%s", dir, mscal_configuration->model_dir);
       }
       } else {
           // Set up the data directory.
           sprintf(mscal_data_directory, "%s/model/%s/data/%s", dir, label, mscal_configuration->model_dir);
    }

    // Can we allocate the model, or parts of it, to memory. If so, we do.
    mscal_velocity_model->dataset_cnt=mscal_configuration->dataset_cnt;
    tempVal = mscal_read_model(mscal_configuration, mscal_velocity_model, mscal_data_directory);

    if (tempVal == SUCCESS) {
//        fprintf(stderr, "WARNING: Could not load model into memory. Reading the model from the\n");
//        fprintf(stderr, "hard disk may result in slow performance.\n");
    } else if (tempVal == FAIL) {
        mscal_print_error("No model file was found to read from.");
        return FAIL;
    }

    // setup config_string 
    sprintf(mscal_config_string,"config = %s\n",configbuf);
    mscal_config_sz=1;

    // Let everyone know that we are initialized and ready for business.
    mscal_is_initialized = 1;

    return SUCCESS;
}

/**
 * Queries mscal at the given points and returns the data that it finds.
 *
 * @param points The points at which the queries will be made.
 * @param data The data that will be returned (Vp, Vs, rho, Qs, and/or Qp).
 * @param numpoints The total number of points to query.
 * @return SUCCESS or FAIL.
 */
int mscal_query(mscal_point_t *points, mscal_properties_t *data, int numpoints) {
    int data_idx = 0;
    nc_type vtype=NC_FLOAT;

    /* iterate through the dataset to see where does the point fall into */
    /* for now assume there is only 1 dataset */
    mscal_dataset_t *dataset=&(mscal_velocity_model->datasets[data_idx]);

    float *lon_list=dataset->longitudes;
    float *lat_list=dataset->latitudes;
    float *dep_list=dataset->depths;
    int nx=dataset->nx;
    int ny=dataset->ny;
    int nz=dataset->nz;

    // hold all the final result
    float *vp_buffer=NULL;
    float *vs_buffer=NULL;
    float *rho_buffer=NULL;

    float *tmp_vp_buffer=NULL;
    float *tmp_vs_buffer=NULL;
    float *tmp_rho_buffer=NULL;

    float lon_f;
    float lat_f;
    float dep_f;
    int lon_idx, first_lon_idx;
    int lat_idx, first_lat_idx;
    int dep_idx, first_dep_idx;
    int same_lon_idx=1;
    int same_lat_idx=1;
    int same_dep_idx=1;

    size_t *dep_idx_buffer;
    size_t *lat_idx_buffer;
    size_t *lon_idx_buffer;

    int offset;

// hold the result
    vp_buffer = malloc(numpoints * sizeof(float));
    if (!vp_buffer) { fprintf(stderr, "malloc failed\n");}
    vs_buffer = malloc(numpoints * sizeof(float));
    if (!vs_buffer) { fprintf(stderr, "malloc failed\n");}
    rho_buffer = malloc(numpoints * sizeof(float));
    if (!rho_buffer) { fprintf(stderr, "malloc failed\n");}

    dep_idx_buffer = malloc(numpoints * sizeof(size_t));
    if (!dep_idx_buffer) { fprintf(stderr, "malloc failed\n");}
    lat_idx_buffer = malloc(numpoints * sizeof(size_t));
    if (!lat_idx_buffer) { fprintf(stderr, "malloc failed\n");}
    lon_idx_buffer = malloc(numpoints * sizeof(size_t));
    if (!lon_idx_buffer) { fprintf(stderr, "malloc failed\n");}

// iterate through all the points and compose the buffers for all inex
    for(int i=0; i<numpoints; i++) {
        data[i].vp = -1;
        data[i].vs = -1;
        data[i].rho = -1;
        data[i].qp = -1;
        data[i].qs = -1;

        lon_f=points[i].longitude;
        lat_f=points[i].latitude;
        dep_f=points[i].depth;

if(mscal_ucvm_debug){ if(i<5) fprintf(stderrfp,"\nfirst %d, float lon/lat/dep = %f/%f/%f\n", i, lon_f, lat_f, dep_f); }

        lon_idx=find_buffer_idx((float *)lon_list,nx,lon_f);
        lat_idx=find_buffer_idx((float *)lat_list,ny,lat_f);
        dep_idx=find_buffer_idx((float *)dep_list,nz,dep_f);
if(mscal_ucvm_debug){ if(i<5) fprintf(stderrfp,"----> depth list is %d  (40)%f (670)%f\n", nz, dep_list[40], dep_list[670]);}
if(mscal_ucvm_debug){ if(i<5) fprintf(stderrfp,"    with idx lon/lat/dep = %d/%d/%d\n", lon_idx, lat_idx, dep_idx); }

        if(i==0) {
            first_dep_idx=dep_idx;
            first_lon_idx=lon_idx;
            first_lat_idx=lat_idx;
        }
        dep_idx_buffer[i]=dep_idx;
        lon_idx_buffer[i]=lon_idx;
        lat_idx_buffer[i]=lat_idx;

	if(dep_idx != first_dep_idx) same_dep_idx=0;
	if(lon_idx != first_lon_idx) same_lon_idx=0;
	if(lat_idx != first_lat_idx) same_lat_idx=0;
    }

// handle access 
// if not TooBig, grab from in memory buffer one at a time
// if tooBig, then collect up all the index list and make just one call and
// retrieve and disperse the result back into data

// it is not too big, extract from data buffers one at a time
    if(!TooBig) { 

        tmp_vp_buffer=dataset->vp_buffer;
        tmp_vs_buffer=dataset->vs_buffer;
        tmp_rho_buffer=dataset->rho_buffer;

        for(int i=0; i<numpoints; i++) {
            dep_idx=dep_idx_buffer[i];
            lat_idx=lat_idx_buffer[i];
            lon_idx=lon_idx_buffer[i];

// offset= (dep_idx)*(lat_cnt * lon_cnt)+(lat_idx)*(lon_cnt)+lon_idx
            offset= (dep_idx)*(ny * nx)+(lat_idx)*(nx)+lon_idx;

if(mscal_ucvm_debug) { fprintf(stderrfp,"\nTarget offset %d : idx lon/lat/dep = %d/%d/%d\n", offset,lon_idx, lat_idx, dep_idx); }

            data[i].vp = ((float *)tmp_vp_buffer)[offset];
            data[i].vs = ((float *)tmp_vs_buffer)[offset];
            data[i].rho = ((float *)tmp_rho_buffer)[offset];
        }

    } else { 

// it is too big, extract data from external data file 
// group netcdf access to 
//   column based(same lat idx an same lon idx),
//   layered(same depth idx) and random method

      if(mscal_ucvm_debug){ fprintf(stderrfp," What is indexing said, same %d %d %d\n", same_dep_idx, same_lon_idx, same_lat_idx); }

      // if same_lon_idx and same_lat_idx,
      // it is depth profile
      //    extract the whole column with dataset->nz for different set 
      //    and then extract data from buffer one at a time using dep_idx 
      if(same_lon_idx && same_lat_idx ) {
        int yesfree=0;
        if( (dataset->col_cache_lat_idx == same_lat_idx) && (dataset->col_cache_lon_idx == same_lon_idx)) {  
            tmp_vp_buffer=dataset->col_cache_vp_buffer;
            tmp_vs_buffer=dataset->col_cache_vs_buffer;
            tmp_rho_buffer=dataset->col_cache_rho_buffer;
            } else { // need to retrieve another set
                tmp_vp_buffer = malloc(nz * sizeof(float));
                if (!tmp_vp_buffer) { fprintf(stderr, "malloc failed\n");}
                cache_depth_col_float(dataset->ncid, dataset->vp_varid,
	  	        nz, first_lat_idx, first_lon_idx, tmp_vp_buffer);
                tmp_vs_buffer = malloc(nz * sizeof(float));
                if (!tmp_vs_buffer) { fprintf(stderr, "malloc failed\n");}
                cache_depth_col_float(dataset->ncid, dataset->vs_varid,
	  	        nz, first_lat_idx, first_lon_idx, tmp_vs_buffer);
                tmp_rho_buffer = malloc(nz * sizeof(float));
                if (!tmp_rho_buffer) { fprintf(stderr, "malloc failed\n");}
                cache_depth_col_float(dataset->ncid, dataset->rho_varid,
	  	        nz, first_lat_idx, first_lon_idx, tmp_rho_buffer);
                yesfree=1;
        }

        for(int i=0; i<numpoints; i++) { 
	    dep_idx=dep_idx_buffer[i];
	    vp_buffer[i]=tmp_vp_buffer[dep_idx];
	    vs_buffer[i]=tmp_vs_buffer[dep_idx];
	    rho_buffer[i]=tmp_rho_buffer[dep_idx];
	}
        if(yesfree) {
            if( dataset->col_cache_lat_idx != -1) {
                free(dataset->col_cache_vp_buffer);
                free(dataset->col_cache_vs_buffer);
                free(dataset->col_cache_rho_buffer);
            }
            dataset->col_cache_vp_buffer=tmp_vp_buffer;
            dataset->col_cache_vs_buffer=tmp_vs_buffer;
            dataset->col_cache_vs_buffer=tmp_rho_buffer;
            dataset->col_cache_lat_idx=first_lat_idx;
            dataset->col_cache_lon_idx=first_lon_idx;
        }

      // if just same_dep_idx, it is a horizontal slice,
      // extract the whole layer using dataset->nx and dataset->ny
      // and then extract data from buffer using lon_idx, and lat_idx
      } else if (same_dep_idx) { 
          int yesfree=0;
          if(dataset->layer_cache_dep_idx == first_dep_idx) { // reues
              tmp_vp_buffer=dataset->layer_cache_vp_buffer;
              tmp_vs_buffer=dataset->layer_cache_vs_buffer;
              tmp_rho_buffer=dataset->layer_cache_rho_buffer;
              } else { // need to grab layer from data file
if(mscal_ucvm_debug) { fprintf(stderrfp," -- calling get a layer with  %d\n",numpoints); }
                  tmp_vp_buffer = malloc((nx*ny) * sizeof(float));
                  tmp_vs_buffer = malloc((nx*ny) * sizeof(float));
                  tmp_rho_buffer = malloc((nx*ny) * sizeof(float));

                  cache_latlon_layer_float(dataset->ncid, dataset->vp_varid,
		      first_dep_idx, ny, nx, tmp_vp_buffer);
                  cache_latlon_layer_float(dataset->ncid, dataset->vs_varid,
		      first_dep_idx, ny, nx, tmp_vs_buffer);
                  cache_latlon_layer_float(dataset->ncid, dataset->rho_varid,
		      first_dep_idx, ny, nx, tmp_rho_buffer);
                  yesfree=1;
          }         

          for(int i=0; i<numpoints; i++) { 
              lat_idx=lat_idx_buffer[i];
	      lon_idx=lon_idx_buffer[i];
	      offset= (lat_idx * nx) + lon_idx;

	      vp_buffer[i]=tmp_vp_buffer[offset];
	      vs_buffer[i]=tmp_vs_buffer[offset];
	      rho_buffer[i]=tmp_rho_buffer[offset];
          }
          if(yesfree) {
              if(dataset->layer_cache_dep_idx != -1) {
                  free(dataset->layer_cache_vp_buffer);
                  free(dataset->layer_cache_vs_buffer);
                  free(dataset->layer_cache_rho_buffer);
              }
              dataset->layer_cache_dep_idx = first_dep_idx;
              dataset->layer_cache_vp_buffer=tmp_vp_buffer;
              dataset->layer_cache_vs_buffer=tmp_vs_buffer;
              dataset->layer_cache_rho_buffer=tmp_rho_buffer;
          }         
      } else {  
// it is so random and so just default to per location access
        for(int i=0; i<numpoints; i++) { 
           lat_idx=lat_idx_buffer[i];
           lon_idx=lon_idx_buffer[i];
           dep_idx=dep_idx_buffer[i];
           vp_buffer[i]=get_nc_vara_float(dataset->ncid, dataset->vp_varid, dep_idx, lat_idx, lon_idx);
           vs_buffer[i]=get_nc_vara_float(dataset->ncid, dataset->vp_varid, dep_idx, lat_idx, lon_idx);
           rho_buffer[i]=get_nc_vara_float(dataset->ncid, dataset->vp_varid, dep_idx, lat_idx, lon_idx);
        }
      }


// finally copy the data over
      for(int i=0; i<numpoints; i++) {
        data[i].vp = ((float *) vp_buffer)[i];
        data[i].vs = ((float *) vs_buffer)[i];
        data[i].rho = ((float *) rho_buffer)[i];
      }

      free(dep_idx_buffer);
      free(lat_idx_buffer);
      free(lon_idx_buffer);
      free(vp_buffer);
      free(vs_buffer);
      free(rho_buffer);
    }


    return SUCCESS;
}

/**
 */
void mscal_setdebug() {
   mscal_ucvm_debug=1;
}               

/**
 * Called when the model is being discarded. Free all variables.
 *
 * @return SUCCESS
 */
int mscal_finalize() {

    mscal_is_initialized = 0;

    if (mscal_configuration) {
        mscal_configuration_finalize(mscal_configuration);
    }
    if (mscal_velocity_model) {
        mscal_velocity_model_finalize(mscal_velocity_model);
    }

    if(mscal_ucvm_debug) {
     fprintf(stderrfp,"DONE:\n"); 
     fclose(stderrfp);
    }

    return SUCCESS;
}

/**
 * Returns the version information.
 *
 * @param ver Version string to return.
 * @param len Maximum length of buffer.
 * @return Zero
 */
int mscal_version(char *ver, int len)
{
  int verlen;
  verlen = strlen(mscal_version_string);
  if (verlen > len - 1) {
    verlen = len - 1;
  }
  memset(ver, 0, len);
  strncpy(ver, mscal_version_string, verlen);
  return 0;
}

/**
 * Returns the model config information.
 *
 * @param key Config key string to return.
 * @param sz number of config terms.
 * @return Zero
 */
int mscal_config(char **config, int *sz)
{
  int len=strlen(mscal_config_string);
  if(len > 0) {
    *config=mscal_config_string;
    *sz=mscal_config_sz;
    return SUCCESS;
  }
  return FAIL;
}


/**
 * Reads the mscal_configuration file describing the various properties of MSCAL and populates
 * the mscal_configuration struct. This assumes mscal_configuration has been "calloc'ed" and validates
 * that each value is not zero at the end.
 *
 * @param file The mscal_configuration file location on disk to read.
 * @param config The mscal_configuration struct to which the data should be written.
 * @return Number of dataset expected in the model
 */
int mscal_read_configuration(char *file, mscal_configuration_t *config) {
    FILE *fp = fopen(file, "r");
    char key[40];
    char value[100];
    char line_holder[128];
    config->dataset_cnt=0;

    // If our file pointer is null, an error has occurred. Return fail.
    if (fp == NULL) { return UCVM_MODEL_CODE_ERROR; }

    // Read the lines in the mscal_configuration file.
    while (fgets(line_holder, sizeof(line_holder), fp) != NULL) {
        if (line_holder[0] != '#' && line_holder[0] != ' ' && line_holder[0] != '\n') {

	    _splitline(line_holder, key, value);

            // Which variable are we editing?
            if (strcmp(key, "utm_zone") == 0) config->utm_zone = atoi(value);
            if (strcmp(key, "model_dir") == 0) sprintf(config->model_dir, "%s", value);
            if (strcmp(key, "interpolation") == 0) { 
                config->interpolation=0;
                if (strcmp(value,"on") == 0) config->interpolation=1;
            }
	    /* for each dataset, allocate a model dataset's block and fill in */ 
            if (strcmp(key, "data_file") == 0) { 
                if( config->dataset_cnt < MSCAL_DATASET_MAX) {
		    int rc=_setup_a_dataset(config,value);
	            if(rc == 1 ) { config->dataset_cnt++; }
                    } else {
                        mscal_print_error("Exceeded dataset maximum limit.");
                }
            }
        }
    }

    // Have we set up all mscal_configuration parameters?
    if (config->utm_zone == 0 || config->model_dir[0] == '\0' || config->dataset_cnt == 0 ) {
        mscal_print_error("One mscal_configuration parameter not specified. Please check your mscal_configuration file.");
        return UCVM_MODEL_CODE_ERROR;
    }

    fclose(fp);
    return UCVM_MODEL_CODE_SUCCESS;
}

/**
 * Extract mscal netcdf dataset specific info
 * and fill in the model info, one dataset at a time
 * and allocate required memory space
 *
 * @param 
 */
int _setup_a_dataset(mscal_configuration_t *config, char *blobstr) {
    /* parse the blob and grab the meta data */
    cJSON *confjson=cJSON_Parse(blobstr);
    int idx=config->dataset_cnt;

    /* grab the related netcdf file and extract dataset info */
    if(confjson == NULL) {
        const char *eptr = cJSON_GetErrorPtr();
        if(eptr != NULL) {
            if(mscal_ucvm_debug){ fprintf(stderrfp, "Configuration dataset setup error: (%s)\n", eptr); }
            return FAIL;
        }
    }

    cJSON *label = cJSON_GetObjectItemCaseSensitive(confjson, "LABEL");
    if(cJSON_IsString(label)){
        config->dataset_labels[idx]=strdup(label->valuestring);
    }
    cJSON *file = cJSON_GetObjectItemCaseSensitive(confjson, "FILE");
    if(cJSON_IsString(file)){
        config->dataset_files[idx]=strdup(file->valuestring);
    }

    return 1;
}

void _trimLast(char *str, char m) {
  int i;
  i = strlen(str);
  while (str[i-1] == m) {
    str[i-1] = '\0';
    i = i - 1;
  }
  return;
}

void _splitline(char* lptr, char key[], char value[]) {

  char *kptr, *vptr;

  for(int i=0; i<strlen(key); i++) { key[i]='\0'; }

  _trimLast(lptr,'\n');
  vptr = strchr(lptr, '=');
  int pos=vptr - lptr;

// skip space in key token from the back
  while ( lptr[pos-1]  == ' ') {
    pos--;
  }

  strncpy(key,lptr, pos);
  key[pos] = '\0';

  vptr++;
  while( vptr[0] == ' ' ) {
    vptr++;
  }
  strcpy(value,vptr);
  _trimLast(value,' ');
}


/*
 * setup the dataset's content 
 * and fill in the model info, one dataset at a time
 * and allocate required memory space
 *
 * */
int mscal_read_model(mscal_configuration_t *config, mscal_model_t *model, char *datadir) {

    int max_idx=model->dataset_cnt; // how many datasets are there
    size_t nelems= 0;
    nc_type vtype;


    char filepath[256];
    for(int i=0; i<max_idx;i++) { 
        mscal_dataset_t *data= &(model->datasets[i]);

        data->vp_buffer=NULL;
        data->vs_buffer=NULL;
        data->rho_buffer=NULL;

        /* find the netcdf data file, no need to save it */
        sprintf(filepath, "%s/%s", datadir, config->dataset_files[i]);
	if(mscal_ucvm_debug) fprintf(stderrfp," data file ..%s\n", filepath);

        /* setup ncid */
        data->ncid=open_nc(filepath);
  
        /* setup nx/ny/nz and void ptrs */
        data->longitudes=(float *) get_nc_buffer(data->ncid, "longitude", filepath, &vtype, &nelems, 1);
        data->nx=nelems;
        if(vtype != NC_FLOAT) { fprintf(stderr,"BADDD.. vtype panic"); }

        data->latitudes=(float *) get_nc_buffer(data->ncid, "latitude", filepath, &vtype, &nelems, 1);
        data->ny=nelems;
        if(vtype != NC_FLOAT) { fprintf(stderr,"BADDD.. vtype panic"); }

        data->depths=(float *) get_nc_buffer(data->ncid, "depth", filepath, &vtype, &nelems, 1);
        data->nz=nelems;
        if(vtype != NC_FLOAT) { fprintf(stderr,"BADDD.. vtype panic"); }

	/* Get variable ID by name */
        data->vp_varid=get_nc_varid(data->ncid,"vp",filepath);
        data->vs_varid=get_nc_varid(data->ncid,"vs",filepath);
        data->rho_varid=get_nc_varid(data->ncid,"rho",filepath);

        /* load the vp/vs/rho in memory  TODO: figure what happen if too big */
        if(!TooBig) {
          data->vp_buffer=get_nc_buffer(data->ncid, "vp", filepath, &vtype, &nelems, 3);
          if(vtype != NC_FLOAT) { fprintf(stderr,"BADDD.. vtype panic"); }
          if(data->vp_buffer == 0) { fprintf(stderr,"PANIC malloc vs\n"); return FAIL; }
	  data->elems=nelems;

          data->vs_buffer=get_nc_buffer(data->ncid, "vs", filepath, &vtype, &nelems, 3);
          if(vtype != NC_FLOAT) { fprintf(stderr,"BADDD.. vtype panic"); }
          if(data->vs_buffer == 0) { fprintf(stderr,"PANIC malloc vs\n"); return FAIL; }
          if(data->elems != nelems) { fprintf(stderr,"BADDD.. nelems panic"); }

          data->rho_buffer=get_nc_buffer(data->ncid, "rho", filepath, &vtype, &nelems, 3);
          if(vtype != NC_FLOAT) { fprintf(stderr,"BADDD.. vtype panic"); }
          if(data->rho_buffer == 0) { fprintf(stderr,"PANIC malloc rho\n");  return FAIL; }
          if(data->elems != nelems) { fprintf(stderr,"BADDD.. nelems panic"); }
        }
        data->layer_cache_dep_idx=-1;
        data->col_cache_lat_idx=-1;
        data->col_cache_lon_idx=-1;

    }
    return SUCCESS;
}

/**
 * Called to clear out the allocated memory 
 *
 * @param 
 */
int mscal_configuration_finalize(mscal_configuration_t *config) {
    int max_idx=config->dataset_cnt; // how many datasets are there
    for(int i=0; i<max_idx;i++) { 
	free(config->dataset_labels[i]);  
	free(config->dataset_files[i]);  
    }
    free(config);
    return SUCCESS;
}

/**
 * Called to clear out the allocated memory 
 *
 * @param 
 */
int mscal_velocity_model_finalize(mscal_model_t *model) {
    int max_idx=model->dataset_cnt; // how many datasets are there
    for(int i=0; i<max_idx; i++) {
        mscal_dataset_t *data= &(model->datasets[i]);
        free(data->depths);
        free(data->latitudes);
        free(data->longitudes);
	free(data->vp_buffer);
	free(data->vs_buffer);
	free(data->rho_buffer);
	nc_close(data->ncid);
    }
    free(model);
    return SUCCESS;
}

/**
 * Prints the error string provided.
 *
 * @param err The error string to print out to stderr.
 */
void mscal_print_error(char *err) {
    fprintf(stderr, "An error has occurred while executing mscal. The error was: \n\n%s\n",err);
    fprintf(stderr, "\nPlease contact software@scec.org and describe both the error and a bit\n");
    fprintf(stderr, "about the computer you are running mscal on (Linux, Mac, etc.).\n");
}

/*
 * Check if the data is too big to be loaded internally (exceed maximum
 * allowable by a INT variable)
 *
 */
static int too_big(mscal_dataset_t *dataset) {
    long max_size= (long) (dataset->nx) * dataset->ny * dataset->nz;
    long delta= max_size - INT_MAX;

    if( delta > 0) {
        return 1;
        } else {
            return 0;
        }
}

// The following functions are for dynamic library mode. If we are compiling
// a static library, these functions must be disabled to avoid conflicts.
#ifdef DYNAMIC_LIBRARY

/**
 * Init function loaded and called by the UCVM library. Calls mscal_init.
 *
 * @param dir The directory in which UCVM is installed.
 * @return Success or failure.
 */
int model_init(const char *dir, const char *label) {
    return mscal_init(dir, label);
}

/**
 * Query function loaded and called by the UCVM library. Calls mscal_query.
 *
 * @param points The basic_point_t array containing the points.
 * @param data The basic_properties_t array containing the material properties returned.
 * @param numpoints The number of points in the array.
 * @return Success or fail.
 */
int model_query(mscal_point_t *points, mscal_properties_t *data, int numpoints) {
    return mscal_query(points, data, numpoints);
}

/**
 * Finalize function loaded and called by the UCVM library. Calls mscal_finalize.
 *
 * @return Success
 */
int model_finalize() {
    return mscal_finalize();
}

/**
 * Version function loaded and called by the UCVM library. Calls mscal_version.
 *
 * @param ver Version string to return.
 * @param len Maximum length of buffer.
 * @return Zero
 */
int model_version(char *ver, int len) {
    return mscal_version(ver, len);
}

/**
 * Version function loaded and called by the UCVM library. Calls mscal_config.
 *
 * @param config Config string to return.
 * @param sz number of config terms
 * @return Zero
 */
int model_config(char **config, int *sz) {
    return mscal_config(config, sz);
}


int (*get_model_init())(const char *, const char *) {
        return &mscal_init;
}
int (*get_model_query())(mscal_point_t *, mscal_properties_t *, int) {
         return &mscal_query;
}
int (*get_model_finalize())() {
         return &mscal_finalize;
}
int (*get_model_version())(char *, int) {
         return &mscal_version;
}
int (*get_model_config())(char **, int*) {
    return &mscal_config;
}



#endif

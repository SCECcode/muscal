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

int mscal_debug=0;

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

    // Initialize variables.
    mscal_configuration = calloc(1, sizeof(mscal_configuration_t));
    mscal_velocity_model = calloc(1, sizeof(mscal_model_t));

    mscal_config_string = calloc(MSCAL_CONFIG_MAX, sizeof(char));
    mscal_config_string[0]='\0';
    mscal_config_sz=0;

    // Configuration file location.
    sprintf(configbuf, "%s/model/%s/data/config", dir, label);

    // Read the mscal_configuration file.
    if (mscal_read_configuration(configbuf, mscal_configuration) != SUCCESS)
XXX
        return FAIL;

    // Set up the data directory.
    sprintf(mscal_data_directory, "%s/model/%s/data/%s/", dir, label, mscal_configuration->model_dir);

    // Can we allocate the model, or parts of it, to memory. If so, we do.
    tempVal = mscal_try_reading_model(mscal_velocity_model);
XXX

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
    int i = 0;

    data[i].vp = -1;
    data[i].vs = -1;
    data[i].rho = -1;
    data[i].qp = -1;
    data[i].qs = -1;

    return SUCCESS;
}

/**
 */
void mscal_setdebug() {
   mscal_debug=1;
}               

/**
 * Called when the model is being discarded. Free all variables.
 *
 * @return SUCCESS
 */
int mscal_finalize() {

    proj_destroy(mscal_geo2utm);
    mscal_geo2utm = NULL;

    if (mscal_velocity_model) free(mscal_velocity_model);
    if (mscal_configuration) free(mscal_configuration);

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
 * @return Success or failure, depending on if file was read successfully.
 */
int mscal_read_configuration(char *file, mscal_configuration_t *config) {
    FILE *fp = fopen(file, "r");
    char key[40];
    char value[100];
    char line_holder[128];

    // If our file pointer is null, an error has occurred. Return fail.
    if (fp == NULL) {
        mscal_print_error("Could not open the mscal_configuration file.");
        return FAIL;
    }

    config->dataset_cnt = 0 ; 
    // Read the lines in the mscal_configuration file.
    while (fgets(line_holder, sizeof(line_holder), fp) != NULL) {
        if (line_holder[0] != '#' && line_holder[0] != ' ' && line_holder[0] != '\n') {
            sscanf(line_holder, "%s = %s", key, value);

            // Which variable are we editing?
            if (strcmp(key, "utm_zone") == 0) config->utm_zone = atoi(value);
            if (strcmp(key, "model_dir") == 0) sprintf(config->model_dir, "%s", value);
            if (strcmp(key, "interpolation") == 0) { 
                config->interpolation=0;
                if (strcmp(value,"on") == 0) config->interpolation=1;
            }
	    /* for each data_file, allocate a mscal configurtion dataset's block and fill in */ 
            if (strcmp(key, "data_file") == 0) { 
                if( config->dataset_cnt == MSCAL_DATASET_MAX) {
                    mscal_print_error("Exceeded dataset maximum limit.");
                    } else {
		        int rc=_mscal_nc_dataset_init(config->dataset_cnt, value);
	                if(rc == 1 ) { config->dataset_cnt++; }
                }
            }
        }
    }

    // Have we set up all mscal_configuration parameters?
    if (config->utm_zone == 0 || config->model_dir[0] == '\0' || config->dataset_cnt == 0 ) {
        mscal_print_error("One mscal_configuration parameter not specified. Please check your mscal_configuration file.");
        return FAIL;
    }

    fclose(fp);

    return SUCCESS;
}

/**
 * Extract mscal netcdf dataest specific info
 * and fill in the model info, one dataset at a time
 * and allocate required memory space
 *
 * @param 
 */
int _mscal_nc_dataset_init(int idx, char *blob) {
	XXX
   /* parse the blob and grab the meta data */
   /* grab the related netcdf file and extract dataset info */
   /* load the vp/vs/rho in memory  
         TODO: figure what happen if too big */
   return SUCCESS;
}

/**
 * Called to clear out the allocated memory 
 *
 * @param 
 */
int _mscal_nc_dataset_finalize(int idx) {
	XXX
   return SUCCESS;
}

/**
 * Prints the error string provided.
 *
 * @param err The error string to print out to stderr.
 */
void mscal_print_error(char *err) {
    fprintf(stderr, "An error has occurred while executing mscal. The error was: %s\n",err);
    fprintf(stderr, "\n\nPlease contact software@scec.org and describe both the error and a bit\n");
    fprintf(stderr, "about the computer you are running mscal on (Linux, Mac, etc.).\n");
}

/**
 * Check if the data is too big to be loaded internally (exceed maximum
 * allowable by a INT variable)
 *
 */
static int too_big() {
        long max_size= (long) (mscal_configuration->nx) * mscal_configuration->ny * mscal_configuration->nz;
        long delta= max_size - INT_MAX;

    if( delta > 0) {
        return 1;
        } else {
        return 0;
        }
}

/**
 * Tries to read the model into memory.
 *
 * @param model The model parameter struct which will hold the pointers to the data either on disk or in memory.
 * @return 2 if all files are read to memory, SUCCESS if file is found but at least 1
 * is not in memory, FAIL if no file found.
 */
int mscal_try_reading_model(mscal_model_t *model) {
    double base_malloc = mscal_configuration->nx * mscal_configuration->ny * mscal_configuration->nz * sizeof(float);
    int file_count = 0;
    int all_read_to_memory =0;
    char current_file[128];
    FILE *fp;

    // Let's see what data we actually have.
    sprintf(current_file, "%s/vp.dat", mscal_data_directory);
    if (access(current_file, R_OK) == 0) {
                if( !too_big() ) { // only if fit
            model->vp = malloc(base_malloc);
            if (model->vp != NULL) {
            // Read the model in.
            fp = fopen(current_file, "rb");
            fread(model->vp, 1, base_malloc, fp);
                        all_read_to_memory++;
            fclose(fp);
            model->vp_status = 2;
            } else {
            model->vp = fopen(current_file, "rb");
            model->vp_status = 1;
                    }
        } else {
            model->vp = fopen(current_file, "rb");
            model->vp_status = 1;
                }
        file_count++;
    }

    sprintf(current_file, "%s/vs.dat", mscal_data_directory);
    if (access(current_file, R_OK) == 0) {
                if( !too_big( )) { // only if fit
            model->vs = malloc(base_malloc);
            if (model->vs != NULL) {
            // Read the model in.
            fp = fopen(current_file, "rb");
            fread(model->vs, 1, base_malloc, fp);
                        all_read_to_memory++;
            fclose(fp);
            model->vs_status = 2;
            } else {
            model->vs = fopen(current_file, "rb");
            model->vs_status = 1;
            }
        } else {
            model->vs = fopen(current_file, "rb");
            model->vs_status = 1;
                }
        file_count++;
    }

    sprintf(current_file, "%s/density.dat", mscal_data_directory);
    if (access(current_file, R_OK) == 0) {
                if(!too_big() ) { // only if fit
            model->rho = malloc(base_malloc);
            if (model->rho != NULL) {
            // Read the model in.
            fp = fopen(current_file, "rb");
            fread(model->rho, 1, base_malloc, fp);
                        all_read_to_memory++;
            fclose(fp);
            model->rho_status = 2;
            } else {
            model->rho = fopen(current_file, "rb");
            model->rho_status = 1;
            }
        } else {
            model->rho = fopen(current_file, "rb");
            model->rho_status = 1;
        }
        file_count++;
    }

    sprintf(current_file, "%s/qp.dat", mscal_data_directory);
    if (access(current_file, R_OK) == 0) {
                if( !too_big() ) { // only if fit
            model->qp = malloc(base_malloc);
            if (model->qp != NULL) {
            // Read the model in.
            fp = fopen(current_file, "rb");
            fread(model->qp, 1, base_malloc, fp);
                        all_read_to_memory++;
            fclose(fp);
            model->qp_status = 2;
            } else {
            model->qp = fopen(current_file, "rb");
            model->qp_status = 1;
            }
        } else {
            model->qp = fopen(current_file, "rb");
            model->qp_status = 1;
        }
        file_count++;
    }

    sprintf(current_file, "%s/qs.dat", mscal_data_directory);
    if (access(current_file, R_OK) == 0) {
                if( !too_big() ) { // only if fit
            model->qs = malloc(base_malloc);
            if (model->qs != NULL) {
            // Read the model in.
            fp = fopen(current_file, "rb");
            fread(model->qs, 1, base_malloc, fp);
                        all_read_to_memory++;
            model->qs_status = 2;
            } else {
            model->qs = fopen(current_file, "rb");
            model->qs_status = 1;
            }
        } else {
            model->qs = fopen(current_file, "rb");
            model->qs_status = 1;
        }
        file_count++;
    }

    if (file_count == 0)
        return FAIL;
    else if (file_count > 0 && all_read_to_memory != file_count)
        return SUCCESS;
    else
        return 2;
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

/**
 * @file mscal.h
 * @brief Main header file for MSCAL library.
 * @version 1.0
 *
 * Delivers the MSCAL model 
 *
 */

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

/** Defines a return value of success */
#define SUCCESS 0
/** Defines a return value of failure */
#define FAIL 1

/* config string */
#define MSCAL_CONFIG_MAX 1000
#define MSCAL_DATASET_MAX 10

// Structures
/** Defines a point (latitude, longitude, and depth) in WGS84 format */
typedef struct mscal_point_t {
	/** Longitude member of the point */
	double longitude;
	/** Latitude member of the point */
	double latitude;
	/** Depth member of the point */
	double depth;
} mscal_point_t;

/** Defines the material properties this model will retrieve. */
typedef struct mscal_properties_t {
	/** P-wave velocity in meters per second */
	double vp;
	/** S-wave velocity in meters per second */
	double vs;
	/** Density in g/m^3 */
	double rho;
	/** Qp */
	double qp;
	/** Qs */
	double qs;
} mscal_properties_t;

/**
Dimensions: 3
  dim[0] name=depth len=84
  dim[1] name=latitude len=381
  dim[2] name=longitude len=471
Total elements: 15073884
**/

/** The MSCAL configuration structure. */
typedef struct mscal_dataset_t {
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

	int elems;
	float *vp_buffer;
	float *vs_buffer;
	float *rho_buffer;

} mscal_dataset_t;

typedef struct mscal_configuration_t {
	/** The zone of UTM projection */
	int utm_zone;
	/** The model directory */
	char model_dir[128];
        /** GTL on or off (1 or 0) */
        int gtl;
	/** interpolation (1 or 0) */
	int interpolation;

        /* how many datasets are in the model */
        int dataset_cnt;
        char *dataset_files[MSCAL_DATASET_MAX];  //strdup
	char *dataset_labels[MSCAL_DATASET_MAX]; // strdup
} mscal_configuration_t;

typedef struct mscal_model_t {
        int dataset_cnt;
        mscal_dataset_t datasets[MSCAL_DATASET_MAX];
} mscal_model_t;


// Constants
/** The version of the model. */
extern const char *mscal_version_string;

// Variables
/** Set to 1 when the model is ready for query. */
extern int mscal_is_initialized;

/** Configuration parameters. */
extern mscal_configuration_t *mscal_configuration;

/** Holds pointers to the velocity model data OR indicates it can be read from file. */
extern mscal_model_t *mscal_velocity_model;

// UCVM API Required Functions

#ifdef DYNAMIC_LIBRARY

/** Initializes the model */
int model_init(const char *dir, const char *label);
/** Cleans up the model (frees memory, etc.) */
int model_finalize();
/** Returns version information */
int model_version(char *ver, int len);
/** Queries the model */
int model_query(mscal_point_t *points, mscal_properties_t *data, int numpts);

int (*get_model_init())(const char *, const char *);
int (*get_model_query())(mscal_point_t *, mscal_properties_t *, int);
int (*get_model_finalize())();
int (*get_model_version())(char *, int);

#endif

// MSCAL Related Functions

/** Initializes the model */
int mscal_init(const char *dir, const char *label);
/** Cleans up the model (frees memory, etc.) */
int mscal_finalize();
/** Returns version information */
int mscal_version(char *ver, int len);
/** Queries the model */
int mscal_query(mscal_point_t *points, mscal_properties_t *data, int numpts);

// Non-UCVM Helper Functions
/** Reads the configuration file. */
int mscal_read_configuration(char *file, mscal_configuration_t *config);
/** Prints out the error string. */
void mscal_print_error(char *err);
/** Retrieves the value at a specified grid point in the model. */
void mscal_read_properties(int x, int y, int z, mscal_properties_t *data);
/** Attempts to malloc the model size in memory and read it in. */
int mscal_read_model(mscal_configuration_t *config, mscal_model_t *model, char* dir);
/** toggle debug flag **/
void mscal_setdebug();
/** free the allocated memory **/
int mscal_configuration_finalize(mscal_configuration_t *config);
/** free the allocated memory **/
int mscal_velocity_model_finalize(mscal_model_t *model);
/** parse JSON metadata blob per dataset **/
int _setup_a_dataset(mscal_configuration_t *conf, char *blobstr);
void _trimLast(char *str, char m);
void _splitline(char* lptr, char key[], char value[]);

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
typedef struct mscal_configuration_t {
	/** The zone of UTM projection */
	int utm_zone;
	/** The model directory */
	char model_dir[128];
        /** GTL on or off (1 or 0) */
        int gtl;
	/** Number of x points */
	int nx;
	/** Number of y points */
	int ny;
	/** Number of z points */
	int nz;
	/** Depth in meters */
	double depth;
	/** list of latitudes **/
	float *latitude;
	/** list of longitudes **/
	float *longitude;
	/** list of depths **/
	float *depths;
} mscal_configuration_t;


/** The model structure which points to available portions of the model. */
typedef struct mscal_model_t {
	/** A pointer to the Vs data either in memory or disk. Null if does not exist. */
	void *vs;
	/** Vs status: 0 = not found, 1 = found and not in memory, 2 = found and in memory */
	int vs_status;
	/** A pointer to the Vp data either in memory or disk. Null if does not exist. */
	void *vp;
	/** Vp status: 0 = not found, 1 = found and not in memory, 2 = found and in memory */
	int vp_status;
	/** A pointer to the rho data either in memory or disk. Null if does not exist. */
	void *rho;
	/** Rho status: 0 = not found, 1 = found and not in memory, 2 = found and in memory */
	int rho_status;
	/** A pointer to the Qp data either in memory or disk. Null if does not exist. */
	void *qp;
	/** Qp status: 0 = not found, 1 = found and not in memory, 2 = found and in memory */
	int qp_status;
	/** A pointer to the Qs data either in memory or disk. Null if does not exist. */
	void *qs;
	/** Qs status: 0 = not found, 1 = found and not in memory, 2 = found and in memory */
	int qs_status;
} mscal_model_t;

// Constants
/** The version of the model. */
const char *mscal_version_string = "MSCAL";

// Variables
/** Set to 1 when the model is ready for query. */
int mscal_is_initialized = 0;

/** Configuration parameters. */
mscal_configuration_t *mscal_configuration;

/** Holds pointers to the velocity model data OR indicates it can be read from file. */
mscal_model_t *mscal_velocity_model;

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

// CCA Related Functions

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
int mscal_try_reading_model(mscal_model_t *model);

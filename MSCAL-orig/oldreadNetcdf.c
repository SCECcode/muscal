/*
 * Basic pure C code to read a NetCDF data file.
 * - Opens a NetCDF file
 * - Locates a variable by name
 * - Reads the entire variable into memory
 * - Prints some sample values
 *
 * Build:
 *   cc readNetcdf.c -o readNetcdf -L$UCVM_INSTALL_PATH/lib/netcdf/lib -lnetcdf
 *
 * Run:
 *   ./readNetcdf path/to/file.nc longitude/latitude/depth
 *
 * Requires:
 *   netCDF C library (netcdf-c)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

/* Simple error-handling macro */
#define NC_CHECK(status)                                        \
    do {                                                        \
        if ((status) != NC_NOERR) {                             \
            fprintf(stderr, "NetCDF error: %s\n",               \
                    nc_strerror(status));                       \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    } while (0)

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file.nc> longitude/latitude/depth \n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argv[1];
    const char *varname = argv[2];

    int ncid = -1;
    int varid = -1;
    int ndims = 0;
    int natts = 0;
    nc_type vtype;

    /* Open file (read-only) */
    NC_CHECK(nc_open(path, NC_NOWRITE, &ncid));

    /* Get variable ID by name */
    int status = nc_inq_varid(ncid, varname, &varid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Variable '%s' not found in %s: %s\n",
                varname, path, nc_strerror(status));
        nc_close(ncid);
        return EXIT_FAILURE;
    }

    /* Inquire variable metadata */
    NC_CHECK(nc_inq_var(ncid, varid, NULL, &vtype, &ndims, NULL, &natts));
    if (ndims <= 0) {
        fprintf(stderr, "Variable '%s' has no dimensions (scalar). Reading scalar...\n", varname);
    }

    /* Get dimension sizes */
    int *dimids = (int *)malloc(sizeof(int) * (ndims > 0 ? ndims : 1));
    if (!dimids) {
        fprintf(stderr, "Out of memory allocating dimids\n");
        nc_close(ncid);
        return EXIT_FAILURE;
    }
    if (ndims > 0) {
        NC_CHECK(nc_inq_var(ncid, varid, NULL, NULL, NULL, dimids, NULL));
    }

    size_t *dimlens = (size_t *)malloc(sizeof(size_t) * (ndims > 0 ? ndims : 1));
    if (!dimlens) {
        fprintf(stderr, "Out of memory allocating dimlens\n");
        free(dimids);
        nc_close(ncid);
        return EXIT_FAILURE;
    }

    size_t nelems = 1;
    for (int i = 0; i < ndims; ++i) {
        size_t len;
        NC_CHECK(nc_inq_dimlen(ncid, dimids[i], &len));
        dimlens[i] = len;
        nelems *= len;
    }

    /* Allocate buffer and read based on variable type */
    void *buffer = NULL;
    size_t elem_size = 0;

    switch (vtype) {
        case NC_BYTE:
        case NC_UBYTE:
            elem_size = sizeof(unsigned char);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_uchar(ncid, varid, (unsigned char*)buffer));
            break;

        case NC_CHAR:
            /* NC_CHAR often represents character arrays / strings */
            elem_size = sizeof(char);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_text(ncid, varid, (char*)buffer));
            break;

        case NC_SHORT:
            elem_size = sizeof(short);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_short(ncid, varid, (short*)buffer));
            break;

        case NC_USHORT:
            elem_size = sizeof(unsigned short);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_ushort(ncid, varid, (unsigned short*)buffer));
            break;

        case NC_INT:
            elem_size = sizeof(int);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_int(ncid, varid, (int*)buffer));
            break;

        case NC_UINT:
            elem_size = sizeof(unsigned int);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_uint(ncid, varid, (unsigned int*)buffer));
            break;

        case NC_INT64:
            elem_size = sizeof(long long);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_longlong(ncid, varid, (long long*)buffer));
            break;

        case NC_UINT64:
            elem_size = sizeof(unsigned long long);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_ulonglong(ncid, varid, (unsigned long long*)buffer));
            break;

        case NC_FLOAT:
            elem_size = sizeof(float);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_float(ncid, varid, (float*)buffer));
            break;

        case NC_DOUBLE:
            elem_size = sizeof(double);
            buffer = malloc(nelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_double(ncid, varid, (double*)buffer));
            break;

        default:
            fprintf(stderr, "Unsupported variable type (type id=%d)\n", vtype);
            goto cleanup;
    }

    /* Print some information and sample values */
    printf("File: %s\n", path);
    printf("Variable: %s\n", varname);
    printf("Type: %d\n", (int)vtype);
    printf("Dimensions: %d\n", ndims);
    for (int i = 0; i < ndims; ++i) {
        char dname[NC_MAX_NAME + 1];
        NC_CHECK(nc_inq_dimname(ncid, dimids[i], dname));
        printf("  dim[%d] name=%s len=%zu\n", i, dname, dimlens[i]);
    }
    printf("Total elements: %zu\n", nelems);

    /* Print first few values (up to 10) */
    size_t to_print = nelems < 10 ? nelems : 10;

    // offset= (dep_idx)*(lat_cnt * lon_cnt)+(lat_idx)*(lon_cnt)+lon_idx

    int offset= nelems < 3695398 ? nelems/2 : 3695398;

    printf("Sample values: ");
    for (size_t i = offset; i < to_print+offset; ++i) {
        switch (vtype) {
            case NC_BYTE:
            case NC_UBYTE:
                printf("%u ", ((unsigned char*)buffer)[i]);
                break;
            case NC_CHAR:
                /* Print as characters; for strings, format may vary */
                printf("%c", ((char*)buffer)[i]);
                break;
            case NC_SHORT:
                printf("%d ", ((short*)buffer)[i]);
                break;
            case NC_USHORT:
                printf("%u ", ((unsigned short*)buffer)[i]);
                break;
            case NC_INT:
                printf("%d ", ((int*)buffer)[i]);
                break;
            case NC_UINT:
                printf("%u ", ((unsigned int*)buffer)[i]);
                break;
            case NC_INT64:
                printf("%lld ", ((long long*)buffer)[i]);
                break;
            case NC_UINT64:
                printf("%llu ", ((unsigned long long*)buffer)[i]);
                break;
            case NC_FLOAT:
                printf("%g ", ((float*)buffer)[i]);
                break;
            case NC_DOUBLE:
                printf("%g ", ((double*)buffer)[i]);
                break;
            default:
                /* already handled earlier */
                break;
        }
    }
    printf("\n");

cleanup:
    if (buffer) free(buffer);
    if (dimids) free(dimids);
    if (dimlens) free(dimlens);
    nc_close(ncid);
    return EXIT_SUCCESS;
}



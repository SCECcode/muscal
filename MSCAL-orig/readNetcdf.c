/*
 *   cc readNetcdf.c -o qreadNetcdf -L$UCVM_INSTALL_PATH/lib/netcdf/lib -lnetcdf
 * Run:
 *   ./readNetcdf path/to/file.nc longitude latitude depth
 */

#include "um_netcdf.h"

const char lon_str[]="longitude";
const char lat_str[]="latitude";
const char dep_str[]="depth";
const char vp_str[]="vp";
const char vs_str[]="vs";
const char rho_str[]="rho";

int main(int argc, char **argv) {
    if (argc !=5) {
        fprintf(stderr, "Usage: %s <file.nc> longitude latitude depth \n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argv[1];

    char *last;
    float lon_f=strtof(argv[2], &last);
    if (argv[2] == last || *last != '\0') {
        fprintf(stderr, "Invalid float: '%s'\n", argv[2]);
        return 1;
    }
    float lat_f=strtof(argv[3], &last);
    if (argv[3] == last || *last != '\0') {
        fprintf(stderr, "Invalid float: '%s'\n", argv[3]);
        return 1;
    }
    float dep_f=strtof(argv[4], &last);
    if (argv[4] == last || *last != '\0') {
        fprintf(stderr, "Invalid float: '%s'\n", argv[4]);
        return 1;
    }

    int ncid = -1;
    size_t nelems= 0;
    nc_type vtype;

    /* Open file (read-only) */
    ncid=open_nc(path);

    /* extract model configuration information - */
    fprintf(stderr," Extract model configuration information \n");

    /* grab for longitude */
    void *lon_list=get_nc_buffer(ncid, lon_str, path, &vtype, &nelems, 1);
    int nx=nelems;

    /* grab for latitude */
    void *lat_list=get_nc_buffer(ncid, lat_str, path, &vtype, &nelems, 1);
    int ny=nelems;

    /* grab for depth */
    void *dep_list=get_nc_buffer(ncid, dep_str, path, &vtype, &nelems, 1);
    int nz=nelems;

    //fprintf(stderr,"  NX=%d\n", nx);
    //fprintf(stderr,"  NY=%d\n", ny);
    //fprintf(stderr,"  NZ=%d\n", nz);
    fprintf(stderr," Query for lon(%f) lat(%f) depth(%f)\n", lon_f, lat_f, dep_f);

    int lon_idx=find_buffer_idx((float *)lon_list,nx,lon_f); 
    //fprintf(stderr,"  lon_idx= %d\n",lon_idx);
    int lat_idx=find_buffer_idx((float *)lat_list,ny,lat_f); 
    //fprintf(stderr,"  lat_idx= %d\n",lat_idx);
    int dep_idx=find_buffer_idx((float *)dep_list,nz,dep_f); 
    //fprintf(stderr,"  dep_idx= %d\n",dep_idx);

    /* Get variable ID by name */
    void *vp_buffer=get_nc_buffer(ncid, vp_str, path, &vtype, &nelems, 3);
    void *vs_buffer=get_nc_buffer(ncid, vs_str, path, &vtype, &nelems, 3);
    void *rho_buffer=get_nc_buffer(ncid, rho_str, path, &vtype, &nelems, 3);

    // offset= (dep_idx)*(lat_cnt * lon_cnt)+(lat_idx)*(lon_cnt)+lon_idx
    // target index (lon,lat,dep)  403 225 20
    //int offset= 3695398;
    int offset= (dep_idx)*(ny * nx)+(lat_idx)*(nx)+lon_idx;
    printf("\n Target offset %d : lon/lat/dep = %d/%d/%d", offset,lon_idx, lat_idx, dep_idx);

    printf("\n             %s is : ", vp_str);
    print_nc_buffer_offset(vtype, offset, vp_buffer);
    printf("\n             %s is : ", vs_str);
    print_nc_buffer_offset(vtype, offset, vs_buffer);
    printf("\n             %s is : ", rho_str);
    print_nc_buffer_offset(vtype, offset, rho_buffer);
    printf("\n");

cleanup:
    if (vp_buffer) free(vp_buffer);
    if (vs_buffer) free(vs_buffer);
    if (rho_buffer) free(rho_buffer);
    if (lon_list) free(lon_list);
    if (lat_list) free(lat_list);
    if (dep_list) free(dep_list);
    nc_close(ncid);
    return EXIT_SUCCESS;
}



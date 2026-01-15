# Autoconf/automake file

objects = um_netcdf.o

# General compiler/linker flags
AM_CFLAGS = ${CFLAGS} -I$(prefix)/include
AM_LDFLAGS = ${LDFLAGS} -L$(prefix)/lib -lm


TARGETS = mscal_query libmscal.a libmscal.so

all: $(TARGETS)

install:
	mkdir -p ${prefix}
	mkdir -p ${prefix}/lib
	mkdir -p ${prefix}/include
	cp libmscal.so ${prefix}/lib
	cp libmscal.a ${prefix}/lib
	cp mscal.h ${prefix}/include
	cp mscal_query ${prefix}/bin

clean:
	rm -rf $(TARGETS)
	rm -rf *.o

libmscal.a: mscal_static.o um_netcdf.o cJSON.o
	$(AR) rcs $@ $^

libmscal.so: mscal.o  um_netcdf.o cJSON.o
	$(CC) -shared $(AM_FCFLAGS) -o libmscal.so $^ $(AM_LDFLAGS)

cJSON.o: cJSON.c
	$(CC) -fPIC -DDYNAMIC_LIBRARY $(AM_CFLAGS) -o $@ -c $^

mscal.o: mscal.c
	$(CC) -fPIC -DDYNAMIC_LIBRARY -o $@ -c $^ $(AM_CFLAGS)

mscal_static.o: mscal.c
	$(CC) -o $@ -c $^ $(AM_CFLAGS)

$(objects): %.o: %.c
	$(CC) -o $@ -c $^ $(AM_CFLAGS)

mscal_query.o: mscal_query.c
	$(CC) $(AM_CFLAGS) -o $@ -c $^

mscal_query : mscal_query.o libmscal.a
	$(CC) -o $@ $^ $(AM_LDFLAGS)



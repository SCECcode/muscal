# Autoconf/automake file

objects = um_netcdf.o muscal_util.o cJSON.o

# General compiler/linker flags
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} -I$(prefix)/include
AM_LDFLAGS = ${LDFLAGS} -L$(prefix)/lib ${LIBS} -lm


TARGETS = muscal_query libmuscal.a libmuscal.so

all: $(TARGETS)

install:
	mkdir -p ${prefix}
	mkdir -p ${prefix}/lib
	mkdir -p ${prefix}/include
	cp libmuscal.so ${prefix}/lib
	cp libmuscal.a ${prefix}/lib
	cp muscal.h ${prefix}/include
	cp muscal_query ${prefix}/bin

clean:
	rm -rf $(TARGETS)
	rm -rf *.o

libmuscal.a: muscal_static.o muscal_util.o um_netcdf.o cJSON.o
	$(AR) rcs $@ $^

libmuscal.so: muscal.o muscal_util.o  um_netcdf.o cJSON.o
	$(CC) -shared $(AM_FCFLAGS) -o libmuscal.so $^ $(AM_LDFLAGS)

muscal.o: muscal.c
	$(CC) -fPIC -DDYNAMIC_LIBRARY -o $@ -c $^ $(AM_CFLAGS)

muscal_static.o: muscal.c
	$(CC) -o $@ -c $^ $(AM_CFLAGS)

$(objects): %.o: %.c
	$(CC) -fPIC -DDYNAMIC_LIBRARY $(AM_CFLAGS) -o $@ -c $^

muscal_query.o: muscal_query.c
	$(CC) $(AM_CFLAGS) -o $@ -c $^

muscal_query : muscal_query.o libmuscal.a
	$(CC) -o $@ $^ $(AM_LDFLAGS)



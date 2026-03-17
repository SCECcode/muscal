# Autoconf/automake file

# General compiler/linker flags
AM_CFLAGS = ${CFLAGS}
AM_FCFLAGS = ${FCFLAGS}
AM_LDFLAGS = ${LDFLAGS}

all:
if WITH_MUSCAL_LARGEDATA_DIR
	echo "USING WITH_MUSCAL_LARGEDATA_DIR in muscal"
else
	echo "NOT USING WITH_MUSCAL_LARGEDATA_DIR in muscal"
	if [ ! -d "muscal" ]; then \
		./make_data_files.py; \
	fi 
endif

install:
if WITH_MUSCAL_LARGEDATA_DIR
	if [ ! -d "${prefix}/data/muscal" ]; then \
		$(MKDIR_P) ${prefix}/data; \
		$(LN_S) -f $(MUSCAL_LARGEDATA_DIR) ${prefix}/data/muscal; \
		echo "USING WITH_MUSCAL_LARGEDATA_DIR in muscal"; \
	fi
	cp config ${prefix}/data
else
	if [ ! -d "${prefix}/data/muscal" ]; then \
		$(MKDIR_P) ${prefix}/data; \
		mv muscal ${prefix}/data; \
		$(LN_S) -f ${prefix}/data/muscal .; \
		echo "NOT USING WITH_MUSCAL_LARGEDATA_DIR in muscal"; \
	fi
	cp config ${prefix}/data
endif

clean:
	echo "clean"

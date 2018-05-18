#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "ddi.h"


void DDI_set_debug( struct ddi_s *ddi, int debug ) {
	ddi->debug = debug;
}

void DDI_set_prefix( struct ddi_s *ddi, char *prefix ) {
	snprintf(ddi->prefix, sizeof(ddi->prefix), "%s", prefix);
	snprintf(ddi->dispatch_name, sizeof(ddi->dispatch_name), "%s.%s", prefix, DDI_OUT_SUFFIX);
	snprintf(ddi->pickup_name, sizeof(ddi->pickup_name), "%s.%s", prefix, DDI_IN_SUFFIX);
	snprintf(ddi->dispatch_tname, sizeof(ddi->dispatch_tname), "%s.t%s", prefix, DDI_OUT_SUFFIX);
	snprintf(ddi->pickup_tname, sizeof(ddi->pickup_tname), "%s.t%s", prefix, DDI_IN_SUFFIX);
}

void DDI_init( struct ddi_s *ddi ) {
	ddi->debug = 0;
	ddi->prefix[0] = 0;
	ddi->mode = DDI_MODE_NONE;
}

void DDI_set_mode( struct ddi_s *ddi, int mode ) {
	ddi->mode = mode;
}

int DDI_dispatch( struct ddi_s *ddi, const char *request ) {
	FILE *fo;
	char *fn, *fnt;

	if (ddi->mode == DDI_MODE_NONE) return 1;

	if (ddi->debug) fprintf(stderr,"DDI Request: '%s'\n", request );

	if (ddi->mode == DDI_MODE_MASTER) {
		fn = ddi->dispatch_name;
		fnt = ddi->dispatch_tname;
	} else if (ddi->mode == DDI_MODE_SLAVE) {
		fn = ddi->pickup_name;
		fnt = ddi->pickup_tname;
	} else {
		// invalid mode
		if (ddi->debug) fprintf(stderr,"Invalid DDI mode (%d)\n", ddi->mode);
		return 2;
	}

	/*
	 * To prevent partial reads of the file, we write to the
	 * temporary file first, then rename it, which is an 
	 * atomic operation
	 */
	fo = fopen(fnt,"w");
	if (fo) {
		fprintf(fo,"%s", request );
		fclose(fo);
		rename(fnt, fn);
	} else {
		if (ddi->debug) fprintf(stderr,"%s:%d: Error trying to open '%s' (%s)\n", __FILE__, __LINE__, fnt, strerror(errno));
		return 1;
	}

	return 0;
}


int DDI_pickup( struct ddi_s *ddi, char *buffer, int bsize ) {
	FILE *f;
	char *fn;

	if (ddi->mode == DDI_MODE_NONE) return 1;

	if (ddi->mode == DDI_MODE_SLAVE) {
		fn = ddi->dispatch_name;
	} else {
		fn = ddi->pickup_name;
	}

	f = fopen(fn, "r");
	if (f) {
		fgets(buffer, bsize, f);
		fclose(f);
		remove(fn);
	} else {
		if (ddi->debug) fprintf(stderr,"%s:%d: Error trying to open '%s' (%s)\n", __FILE__, __LINE__, fn, strerror(errno));
		return 1;
	}

	return 0;
}
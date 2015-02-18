# contrib/pg_audit_trail/Makefile

MODULE_big = pg_audit_trail
OBJS = pg_audit_trail.o $(WIN32RES)
PGFILEDESC = "pg_audit_trail - logging facility for audit events"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_audit_trail
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

include $(top_srcdir)/common.am

# Commented-out clients do not yet use the new API

SUBDIRS =
#SUBDIRS = shell save_button panel

if HAVE_ALSA
SUBDIRS += simple_client
#SUBDIRS += simple_client synth
endif

dist_bin_SCRIPTS = lash_control

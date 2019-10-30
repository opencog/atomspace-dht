
DECLARE_GUILE_CONFIG_TARGET(SCM_CONFIG "opencog dht-config" "ATOMSPACE_TEST")

# This follows the same directory hierarchy as the atomspace git repo.
ADD_SUBDIRECTORY (persist)
ADD_SUBDIRECTORY (scm)

# Boilerplate for unit testing. See the AtomSpace as-config.scm
# file for the prime usage example.
WRITE_GUILE_CONFIG(${GUILE_BIN_DIR}/opencog/dht-config.scm SCM_CONFIG TRUE)
WRITE_GUILE_CONFIG(${GUILE_BIN_DIR}/opencog/dht-config-installable.scm
	SCM_CONFIG FALSE)
INSTALL(FILES ${GUILE_BIN_DIR}/opencog/dht-config-installable.scm
	DESTINATION ${GUILE_SITE_DIR}/opencog RENAME dht-config.scm)

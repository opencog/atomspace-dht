#
# Build the OpenDHT driver for the AtomSpace
#

ADD_LIBRARY (persist-dht SHARED
	DHTAtomDelete
	DHTAtomLoad
	DHTAtomStorage
	DHTAtomStore
	DHTBulk
	DHTIncoming
	DHTValues
	DHTPersistSCM
)

TARGET_LINK_LIBRARIES(persist-dht
	smob
	opendht
	gnutls
	nettle
	argon2
)

ADD_GUILE_EXTENSION(SCM_CONFIG persist-dht "opencog-ext-path-persist-dht")

INSTALL (TARGETS persist-dht EXPORT AtomSpaceTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

# Smell tests.

ADD_EXECUTABLE(sniff sniff)
TARGET_LINK_LIBRARIES(sniff opendht gnutls nettle argon2)

ADD_EXECUTABLE(snuff snuff)
TARGET_LINK_LIBRARIES(snuff opendht gnutls nettle argon2)

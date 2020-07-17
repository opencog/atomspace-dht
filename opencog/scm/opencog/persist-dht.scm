;
; OpenCog OpenDHT Persistence module
;

(define-module (opencog persist-dht))


(use-modules (opencog))
(use-modules (opencog dht-config))
(load-extension
	(string-append opencog-ext-path-persist-dht "libpersist-dht")
	"opencog_persist_dht_init")

(export dht-bootstrap dht-clear-stats dht-close dht-open dht-stats
	dht-examine dht-atomspace-hash dht-immutable-hash dht-atom-hash
	dht-node-info dht-storage-log dht-routing-tables-log dht-searches-log
	dht-load-atomspace)

; --------------------------------------------------------------

(set-procedure-property! dht-atom-hash 'documentation
"
 dht-atom-hash ATOM - Return string holding the DHT hash-key of ATOM.
    The returned string will be a 40-character hex string encoding the
    20-byte hash-key of the DHT entry for the ATOM in the currently
    open AtomSpace. All of the ATOM's Incoming Set and Values are
    attached to this key.

    Example: Print the incoming set and values on an Atom:
       (display (dht-examine (dht-atom-hash (Concept \"foo\"))))
")

(set-procedure-property! dht-atomspace-hash 'documentation
"
 dht-atomspace-hash - Return string holding the AtomSpace DHT hash-key.
    The returned string will be a 40-character hex string encoding the
    20-byte hash-key of the DHT entry for the currently open AtomSpace.

    Example: Print all of the Atoms in the AtomSpace
       (display (dht-examine (dht-atomspace-hash)))
")

(set-procedure-property! dht-bootstrap 'documentation
"
 dht-bootstrap URI - Provide a DHT peer to connect to.
    This will cause the local DHT node to connect to the indicated peer,
    and join the DHT network that it is connected to. More than one peer
    can be specified; all peers should belong to the same network,
    else there is a risk of bridging two unrelated networks.  This
    might reult in chaos.

    The URI should be of one of the following forms:
        dht://hostname/
        dht://hostname:port/
")

(set-procedure-property! dht-clear-stats 'documentation
"
 dht-clear-stats - reset the performance statistics counters.
    This will zero out the various counters used to track the
    performance of the OpenDHT backend.  Statistics will continue to
    be accumulated.
")

(set-procedure-property! dht-close 'documentation
"
 dht-close - close the currently open DHT backend.
    Close open connections to the currently-open backend, after flushing
    any pending writes in the write queues. After the close, atoms can
    no longer be stored to or fetched from the database.
")

(set-procedure-property! dht-examine 'documentation
"
 dht-examine HASH-KEY - Return string describing a DHT entry.
    The HASH-KEY should be a 40-character hex string encoding the
    20-byte hash-key of a DHT entry.

    Example: Print all of the Atoms in the AtomSpace:
       (display (dht-examine (dht-atomspace-hash)))

    Example: Print the incoming set and values on an Atom:
       (display (dht-examine (dht-atom-hash (Concept \"foo\"))))
")

(set-procedure-property! dht-immutable-hash 'documentation
"
 dht-immutable-hash ATOM - Return string w/DHT hash-key encoding of ATOM.
    The returned string will be a 40-character hex string encoding the
    20-byte hash-key of the DHT entry for the ATOM in its immutable
    form: that is, the scheme string representation of the Atom, as
    it is without any attached Values or IncomingSet. This entry
    exists outside of any particular AtomSpace, as it is globally unique
    (precisely because it has no attached Values or IncomingSet.)
    As such, it can be used as the globally-unique handle for the ATOM.

    Example: Print the scheme representation of an Atom:
       (display (dht-examine (dht-immutable-hash (Concept \"foo\"))))
")

(set-procedure-property! dht-node-info 'documentation
"
 dht-node-info - Return string describing the running DHT node.
    This includes information about the number of connected nodes.
")

(set-procedure-property! dht-open 'documentation
"
 dht-open URL - Open a connection to an OpenDHT server.

  The URL must be one of these formats:
     dht:///KEY-NAME
     dht://HOSTNAME/KEY-NAME
     dht://HOSTNAME:PORT/KEY-NAME

  If no hostname is specified, its assumed to be 'localhost'. If no port
  is specified, its assumed to be 5001.

  Examples of use with valid URL's:
     (dht-open \"dht:///atomspace-test\")
     (dht-open \"dht://localhost/atomspace-test\")
     (dht-open \"dht://localhost:5001/atomspace-test\")
")

(set-procedure-property! dht-stats 'documentation
"
 dht-stats - report performance statistics.
    This will cause some database performance statistics to be printed
    to the stdout of the server. These statistics can be quite arcane
    and are useful primarily to the developers of the database backend.
")

(set-procedure-property! dht-storage-log 'documentation
"
 dht-storage-log - Return a string describing the DHT Node storage log.
")

(set-procedure-property! dht-routing-tables-log 'documentation
"
 dht-routing-tables-log - Return string w/the DHT Node routing tables log.
")

(set-procedure-property! dht-searches-log 'documentation
"
 dht-searches-log - Return string w/the DHT Node searches log.
")

(set-procedure-property! dht-load-atomspace 'documentation
"
 dht-load-atomspace PATH - Load all Atoms from the PATH into the AtomSpace.

   For example:
      `(dht-load-atomspace \"QmT9tZttJ4gVZQwVFHWTmJYqYGAAiKEcvW9k98T5syYeYU\")`
   should load the atomspace from `basic.scm` example.  In addition,
   IPNS and IPFS paths are allowed: e.g.
      `(dht-load-atomspace \"/dht/QmT9tZt...\")`
   and
      `(ipns-load-atomspace \"/ipns/QmVkzxh...\")`
   with the last form performing an IPNS resolution to obtain the actual
   IPFS CID to be loaded.

   See also `dht-fetch-atom` for loading individual atoms.
")

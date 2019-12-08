;
; basic-store.scm
;
; Simple example of storing Atoms in the DHT.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-dht))

; Use the atomspace called "test-atomspace" for this demo.
; This will run an OpenDHT node on localhost at the default port 4343.
(dht-open "dht:///test-atomspace")

; --------------------------------------------------------------------
; Alternately: run the DHT node on localhost at port 4545:
; (dht-open "dht://:4545/test-atomspace")
;
; The general URI structure is
;
;     dht://[<hostname>][:<port-number>]/[<Atomspace-name>]
;
; For `dht-open`, the <hostname> MUST be blank; the DHT node is always
; the localhost. To bootstrap to other hosts, keep reading...
;
; Some things about DHT networks you need to know:
;
; * There is a global (planet-wide) OpenDHT network at port 4222, the
;   default port number for OpenDHT. Although it can be used, it is not
;   very suitable for AtomSpace data, as it discards unwanted data after
;   ten minutes. Data is "unwanted" if no one is asking for it. Clearly,
;   this won't work well for AtomSpace data, which might be dormant for
;   a long time.
;
; * Therefore, the AtomSpace driver defines it's own network, defaulting
;   to port 4343. By default, all users of the AtomSpace-DHT driver will
;   connect to this network.  Atomspace data will stay persistent on
;   this network *IF* at least one network node is up (and there's
;   enough RAM). See `dht-node.scm` for an example of how to run such a
;   node.
;
;   Someday (in the future), you will be able to connect to this global
;   network by "bootstrapping" to it.  That means connecting to one (or
;   more) well-known network nodes, after which peer discovery will
;   handle the rest. You can bootstrap to the (non-)"existing" global
;   AtomSpace DHT Network with:
;
;       (dht-bootstrap "dht://bootstrap.opencog.org:4343/")
;
;   For now, you will have to run your own bootstrap network.  Do this
;   by opening a SECOND guile shell (on localhost, or elsewhere) and
;   saying
;
;       (dht-open "dht://:4444/")
;
;   Notice that NO AtomSpace name is supplied! This creates a DHT node
;   listening on port 4444; it will accept and mirror any data for any
;   AtomSpace.  See `dht-node.scm` for details.
;
;   Now, connect to the bootstrap network:

(dht-bootstrap "dht://localhost:4444/")

;   The above assumes that that you ran the boostrap node on this same
;   machine (localhost). If you have second machine, and want to run the
;   demo over the network, start the bootstrap node on that machine, and
;   connect to it with
;
;       (dht-bootstrap "dht://some.other.machine:4444/")
;
;   or you can use an IP address:
;
;       (dht-bootstrap "dht://192.168.1.2:4444/")
;
; * Caution: This demo will work fine, if you don't have a bootstrap
;   node. However, the next demo, `basic-fetch.scm`, will fail without
;   it.  That's because someone, somewhere, has to hold on to the
;   AtomSpace data, after existing from this guile shell. One
;   alternative to having a bootstrap node is to just use *this* shell
;   as the bootstrap node: just keep it open, and run the `basic-fetch`
;   demo in some other guile shell. (Keep in mind that you'll have to
;   adjust the port numbers, if you do this.)
;
; * You can also view AtomSpace data with the `dhtnode` command-line
;   tool, provided by OpenDHT. This can be mildly entertaining.  At
;   a bash prompt, say
;
;       $ dhtnode -b localhost:4444 -n 42
;
;   This will connect to the bootstrap node on localhost.
;   The `-n 42` flag on `dhtnode` tells it to run the "private" DHT
;   network "42".  By default, all AtomSpace data will be marked with
;   this network ID. See `config.dht_config.node_config.network`.
;
;   Keep in mind that `dhtnode` is useless for holding AtomSpace data.
;   Whatever AtomSpace data it receives, it will hold on to it for only
;   ten minutes, and then it will delete it's copy.
;
; --------------------------------------------------------------------

; Store a single atom
(store-atom (Concept "foo" (stv 0.6 0.8)))

; Obtain the DHT key-hash for this atom.  For this Atom in an AtomSpace
; with this particular name, this will be globally unique, and it
; should be equal to 2220eca6560c36cde76315204130dbb182756a9b.
; This 160-bit hash is derived from the Atom type and the Atom name.
(display (dht-atom-hash (ConceptNode "foo"))) (newline)

; The Values and Incoming Set attached to it can be examined:
(display (dht-examine (dht-atom-hash (ConceptNode "foo"))))

; Similarly, the hash of this atomspace is also unique; it should be
; d2bf1fd0312cbf309df74c537bea16769b419f44. This hash is derived from
; the AtomSpace name (the name given in the `dht-open` clause.)
(display (dht-atomspace-hash)) (newline)

; Given an (160-bit DHT) hash that identifies an AtomSpace, the current
; contents of that AtomSpace can be viewed, as they exist in the DHT:
(display (dht-examine "d2bf1fd0312cbf309df74c537bea16769b419f44"))

; AtomSpace backend storage stats can be viewed:
(dht-stats)

; Add some data to the atom.
(cog-set-value! (Concept "foo") (Predicate "floater")
	(FloatValue 1 2 3 4))

(cog-set-value! (Concept "foo") (Predicate "name list")
	(StringValue "Joe" "Smith" "42nd and Main St."))

; Store the new data. The Atom will NOT be pushed out to the DHT until
; `store-atom` is called.  Until then, it exists only in the local
; AtomSpace.
(store-atom (Concept "foo"))

; View the DHT contents, again.
(display (dht-examine (dht-atomspace-hash)))

; Add an EvaluationLink to the AtomSpace; give it a TruthValue
(define e
	(Evaluation (stv 0.2 0.9)
		(Predicate "relation")
		(List (Concept "foo") (Concept "bar"))))

; Store it as well.
(store-atom e)

; Just for grins, show the atomspace contents now:
(display (dht-examine (dht-atomspace-hash)))

; Take a closer look at (Concept "foo")
(display (dht-examine (dht-atom-hash (ConceptNode "foo"))))

; Notice that it had an entry labelled "Incoming".  It should have
; printed like so:
;     Incoming: bc91a93a9a0d046e09cab9b9cea74a16f06c4675
; What is this mystery value?  Let's find out:
(display (dht-examine "bc91a93a9a0d046e09cab9b9cea74a16f06c4675"))

; Oh. It was the (ListLink (ConceptNode "foo") (ConceptNode "bar"))
; That makes sense: "Incoming" is the incoming link (the ListLink).
; But where did this hash-key come from? Why, it's this:
(dht-immutable-hash (ListLink (ConceptNode "foo") (ConceptNode "bar")))

; The immutable hash is different from the atom-hash:
(dht-atom-hash (ListLink (ConceptNode "foo") (ConceptNode "bar")))

; The immutable hash stores the globally-unique scheme string
; representation of the Atom. This string representation is
; independent of the AtomSpace; all AtomSpaces use the same string.
; The "mutable" hash is used to store the Values and IncomingSet of
; an Atom, as these exist in some particular AtomSpace.  The "mutable"
; hash thus incorporates the AtomSpace name; this allows different
; AtomSpaces to hold different (and changing over time) Values and
; IncomingSets for an Atom.

; We are done. Close the connection to the dht table.
(dht-close)

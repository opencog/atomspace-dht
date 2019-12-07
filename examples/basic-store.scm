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
; Some things about DHT networks you need to know:
;
; * There is a global (planet-wide) OpenDHT network at port 4222, the
;   default port number for OpenDHT. Although it can be used, it is not
;   very suitable for AtomSpace data, as it typically discards unwanted
;   data after ten minutes. Data is "unwanted" if no one is asking for
;   it. Clearly, this won't work well for AtomSpace data.
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
; * To avoid polluting the "main" AtomSpace network with debugging junk,
;   it is useful to run a local debug network. In these examples, it
;   will be at port 4444. So: at another bash prompt, start a `dhtnode`:
;
;       $ dhtnode -p 4444 -n 42
;
;   Then bootstrap to it, here:
;
;       (dht-bootstrap "dht://localhost:4444/")
;
;   Debugging can be performed by examining the DHT directly with the
;   `dhtnode` console. Alternately, see `dht-node.scm` for examples
;   of accessing DHT data and status directly from guile/scheme.
;
;   The `-n 42` flag on `dhtnode` tells it to run the "private" DHT
;   network "42".  By default, all AtomSpace data will be marked with
;   this network ID. See `config.dht_config.node_config.network`.
;
; --------------------------------------------------------------------

; Store a single atom
(store-atom (Concept "foo" (stv 0.6 0.8)))

; Obtain the DHT key-hash for this atom.  For this Atom in an AtomSpace
; with this particular name, this will be globally unique, and it
; should be equal to 2220eca6560c36cde76315204130dbb182756a9b.
; This 80-bit hash is derived from the Atom type and the Atom name.
(display (dht-atom-hash (ConceptNode "foo"))) (newline)

; The Values and Incoming Set attached to it can be examined:
(display (dht-examine (dht-atom-hash (ConceptNode "foo"))))

; Similarly, the hash of this atomspace is also unique; it should be
; d2bf1fd0312cbf309df74c537bea16769b419f44. This hash is derived from
; the AtomSpace name (the name given in the `dht-open` clause.)
(display (dht-atomspace-hash)) (newline)

; Given an (80-bit DHT) hash that identifies an AtomSpace, the current
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

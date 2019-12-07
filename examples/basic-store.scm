;
; basic-store.scm
;
; Simple example of storing Atoms in the DHT.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-dht))

; Use the atomspace called "test-atomspace" for this demo.
; This will run an OpenDHT node on localhost at port 4343.
(dht-open "dht:///test-atomspace")

; --------------------------------------------------------------------
; Alternately: run the DHT node on a non-default port
; (dht-open "dht://:4444/test-atomspace")
;
; Alternately: Bootstrap to an existing AtomSpace DHT Network.
; (dht-bootstrap "dht://bootstrap.opencog.org:4343/")
;
; For debugging, it can be useful to run a private dhtnode on the local
; machine. It can be started at the bash prompt with `dhtnode -p 4444`
; In this case, bootstrap to it:
;     (dht-bootstrap "dht://localhost:4444/")
; You can then examine the DHT directly with the `dhtnode` console.
;
; Some things about DHT networks you need to know:
;
; * There is a global (planet-wide) OpenDHT network at port 4222, the
;   default port number for OpenDHT. Although it can be used, it is not
;   very sutiable for AtomSpace data, as it typically discards unwanted
;   data after ten minutes. Data is "unwanted" if no one is asking for
;   it. Clearly, this won't work well for AtomSpace data.
;
; * Therefore, the AtomSpace driver defines it's own network, defaulting
;   to port 4343. By default, all users of the AtomSpace-DHT driver will
;   connect to this network.  Atomspace data will stay persistent on
;   this network *IF* at least one network node is up (and there's
;   enough RAM). See `dht-node.scm` for an example of how to run such a
;   node.

; --------------------------------------------------------------------

; Store a single atom
(store-atom (Concept "foo" (stv 0.6 0.8)))

; Obtain the DHT key-hash for this atom.  For this Atom in an AtomSpace
; with this particular name, this will be globally unique, and it
; should be equal to 2220eca6560c36cde76315204130dbb182756a9b.
(display (dht-atom-hash (ConceptNode "foo")))

; The Values and Incoming Set attached to it can be examined:
(display (dht-examine (dht-atom-hash (ConceptNode "foo"))))

; Similarly, the hash of this atomspace is also unique; it should be
; d2bf1fd0312cbf309df74c537bea16769b419f44.
(display (dht-atomspace-hash))

; Printing this should show the current contents of the AtomSpace,
; as it exists in the DHT.
(display (dht-examine "d2bf1fd0312cbf309df74c537bea16769b419f44"))

; View stats
(dht-stats)

; Add some data to the atom
(cog-set-value! (Concept "foo") (Predicate "floater")
	(FloatValue 1 2 3 4))

(cog-set-value! (Concept "foo") (Predicate "name list")
	(StringValue "Joe" "Smith" "42nd and Main St."))

; Store the new data
(store-atom (Concept "foo"))

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

; Notice that it had an antry labelled "Incoming".  It should have
; printed like so:
; Incoming: 50a541b715203c5e94845aad68c5bd5f92068991
; What is this mystery value?  Let's find out:
(display (dht-examine "50a541b715203c5e94845aad68c5bd5f92068991"))

; Oh. It was the (ListLink (ConceptNode "foo") (ConceptNode "bar"))
; But where did this key come from?
(dht-immutable-hash (ListLink (ConceptNode "foo") (ConceptNode "bar")))

; The immutable hash is different from the atom-hash:
(dht-atom-hash (ListLink (ConceptNode "foo") (ConceptNode "bar")))

; The immutable hash stores the globally-unique scheme string
; representation of the Atom. This string representation is
; independent of the AtomSpace; all AtomSpaces use the same string.
; The mutable hash stores the Values and IncomingSet of an Atom,
; as these exist in some particular AtomSpace.  The Values and the
; IncomingSet change over time, and they will be different, in general,
; for different AtomSpaces.

; We are done. Close the connection to the dht table.
(dht-close)

;
; basic-store.scm
;
; Simple example of storing Atoms in the DHT.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-dht))

; Use the atomspace called "test-atomspace" for this demo.
; This will run an OpenDHT node on localhost at port 4222
(dht-open "dht:///test-atomspace")

; Run the DHT node on a non-default port
; (dht-open "dht://:4444/test-atomspace")

; Bootstrap to an existing AtomSpace DHT Network.
; (dht-bootstrap "dht://bootstrap.opencog.org:4222/")
;
; For debugging, it can be useful to run a private dhtnode on the local
; machine. It can be started at teh bash prompt with `dhtnode -p 4444`
; In this case, bootstrap to it:
;     (dht-bootstrap "dht://localhost:4444/")
; You can then examinethe DHT directly with the dhtnode console.

; Store a single atom
(store-atom (Concept "foo" (stv 0.6 0.8)))

; View stats
(dht-stats)

; Add some data to the atom
(cog-set-value! (Concept "foo") (Predicate "floater")
	(FloatValue 1 2 3 4))

(cog-set-value! (Concept "foo") (Predicate "name list")
	(StringValue "Joe" "Smith" "42nd and Main St."))

; Store the new data
(store-atom (Concept "foo"))

; Close the connection to the dht table.
(dht-close)

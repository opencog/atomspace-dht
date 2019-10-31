;
; basic-store.scm
;
; Simple example of storing Atoms in the DHT.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-dht))

; Open connection of a DHT node assumed to be running on localhost
; This is a bad assumption, unless you've started up such a node.
(dht-open "dht:///test-atomspace")

; Open a DHT node on the localhost, port 4222
; (dht-open "dht://localhost:4222/test-atomspace")

; The below works, but is not that good an idea, since it spams the
; Jami/Ring network with our junk.
; (dht-open "dht://bootstrap.ring.cx:4222/test-atomspace")

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

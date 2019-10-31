;
; basic-fetch.scm
;
; Simple example of fetching previously-stored Atoms from the DHT.
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-dht))

; Open connection of a DHT node assumed to be running on localhost
; This is a bad assumption, unless you've started up such a node.
(dht-open "dht:///test-atomspace")

; Fetch a single atom
(define c (Concept "foo"))
(fetch-atom c)

; Examine the Values that were fetched.
(cog-keys c)

(dht-stats)
(dht-close)

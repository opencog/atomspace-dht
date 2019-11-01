;
; basic-fetch.scm
;
; Simple example of fetching previously-stored Atoms from the DHT.
;
(use-modules (srfi srfi-1))
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-dht))

; Declare th name of the AtomSpace to work with.
(dht-open "dht:///test-atomspace")

; Connect to a DHT peer. This must be a machine on a DHT network that
; contains the test atomspace.  If one cannot connect to this peer,
; then the atomspace won't be found.
(dht-bootstrap "dht://localhost:4444")

; Fetch a single atom
(define c (Concept "foo"))
(fetch-atom c)

; Examine the Values that were fetched.
(cog-keys c)

; Verify that the values matched what was previously stored.
(cog-value c (first (cog-keys c)))
(cog-value c (second (cog-keys c)))
(cog-value c (third (cog-keys c)))

; Get the incoming set -- The previous example placed the ConceptNode
; into a ListLink. Lets get that back.
(fetch-incoming-set c)

; Take a look at what we found.  We should have gotten back a ListLink.
(cog-incoming-set c)

; Do it again -- get the incoming set of the ListLink.  It should
; be the EvaluationLink.
(define ll (first (cog-incoming-set c)))
(fetch-incoming-set ll)

; And ... take a look at the incoming set. The EvaluationLink
; should be there, AND it should have the correct values on it, too.
(cog-incoming-set ll)

(dht-stats)
(dht-close)

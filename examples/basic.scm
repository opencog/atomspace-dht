;
; basic.scm
;
; Simple example
;
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-dht))

; Open connection of a DHT node assumed to be running on localhost
; This is a bad assumption, unless you've started up such a node.
(dht-open "dht:///")

; Open a DHT node on the localhost, port 4222
; (dht-open "dht://localhost:4222/")

; The below works, but is not that good an idea, since it spams the
; Jami/Ring network with our junk.
; (dht-open "dht://bootstrap.ring.cx:4222/")

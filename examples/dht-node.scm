;
; node.scm
;
; Example of running a stand-alone DHT node.
;
; One will typically want to (need to) run a stand-alone DHT network
; node on the local machine.  This is useful for several reasons:
;
;  * It can hold some of the Atomspace data that other DHT users
;    have published.
;
;  * It provides persistence when no outside-world DHT servers are
;    visible or available on the network.
;
;  * Views of the data can help with debugging.
;
(use-modules (srfi srfi-1))
(use-modules (opencog))
(use-modules (opencog persist))
(use-modules (opencog persist-dht))
;
; Start the node on port 4444.  Do NOT specify any AtomSpace!  This
; node will simply listen on the network and cache data, without
; having the active ability to modify AtomSpace contents.
(dht-open "dht://:4444/")
;
; Examine the network stats
(display (dht-node-info))

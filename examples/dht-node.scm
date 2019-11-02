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
;
; A list of the objects stored on this node.
(display (dht-storage-log))
;
; Examine some of them:
(display (dht-examine "0000000000000000000000000000000000000000"))
(display (dht-examine "51fd892f0ee03a96be14322b6c9b02db697c5074"))

; Examine the routing tables:
(display (dht-routing-tables-log))

; Examine the searches log:
(display (dht-searches-log))

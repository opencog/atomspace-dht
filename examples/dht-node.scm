;
; dht-node.scm
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
;  * (TODO) With the ability to listen to updates, such a node could
;    act as a bridge to file-backed storage (e.g. to the SQL backend).
;    Similarly, the file-backed storage could be a seeder for DHT nodes
;    that have expired thier data.
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
(display (dht-examine "ea89a943a6d23e8a4418d7896e25f6ce968df52a"))
(display (dht-examine "d2bf1fd0312cbf309df74c537bea16769b419f44"))

; Examine the routing tables:
(display (dht-routing-tables-log))

; Examine the searches log:
(display (dht-searches-log))

define(
    $nf_id		5,
    $iprw_id	1282,
	$debug		false,
    $queueSize	1024,
	$port		28284,
);

ec :: P4SFCEncap();
rw :: P4IPRewriter($iprw_id, $debug, $port, pattern - - 192.168.0.1-192.168.0.255 - 0 0);

nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(nf_id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(nf_id)_tx, TO_PROC main_rx, IQUEUE $queueSize);

nf_from -> Strip(14)
	-> ec
	-> Print(in, ACTIVE $debug)
	-> CheckIPHeader 
	-> IPPrint(in_ip, ACTIVE $debug)
	-> rw
	-> CheckIPHeader
	-> IPPrint(out_ip, ACTIVE $debug)
	-> [1]ec;

ec[1]   -> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0)
		-> nf_to;

Script( TYPE ACTIVE,
        print "RX: $(nf_from.count), TX: $(nf_to.n_sent)/$(nf_to.n_dropped)",
        wait 1,
  	    loop
        );

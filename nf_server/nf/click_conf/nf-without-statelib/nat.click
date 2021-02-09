define(
      $nf_id      2,
      $queueSize  1024,
      $debug      false
      );

nf_from ::  FromDPDKRing(MEM_POOL 1,  FROM_PROC nf$(nf_id)_rx, TO_PROC main_tx);
nf_to   ::  ToDPDKRing  (MEM_POOL 2,  FROM_PROC nf$(nf_id)_tx, TO_PROC main_rx, IQUEUE $queueSize);

rw :: SampleIPRewriter($debug, pattern 66.66.66.66 10000-65535 - - 0 0, drop);
ec :: P4SFCEncap();

AddressInfo(
  intern 	10.0.0.1	10.0.0.0/8,
  extern	66.66.66.66  00:a0:b0:c0:d0:e0,
  extern_next_hop	00:10:20:30:40:50,
);

ip :: IPClassifier(src net intern and dst net intern,
                   src net intern,
                   dst host extern,
                   -);

nf_from -> Print(in, ACTIVE $debug)
	-> Strip(14)
	-> ec
      -> Print(in_decap, ACTIVE $debug)
      -> CheckIPHeader
      -> Print(after_checkiph, ACTIVE $debug)
      -> IPPrint(in_ip, ACTIVE $debug)
      -> ip; 

ip[0] -> [1]ec;
ip[1] -> Print(ip2rw, ACTIVE $debug) -> [0]rw;
ip[2] -> [1]rw;
ip[3] -> [1]rw;

rw[0] -> IPPrint(out_ip, ACTIVE $debug)
      -> Print(before_p4sfcencap, ACTIVE $debug)
      -> [1]ec;

ec[1] -> Print(before_eth, ACTIVE $debug)
      -> EtherEncap(0x1234, extern:eth, extern_next_hop:eth)
      -> Print(out, ACTIVE $debug)
      -> nf_to;

Script( TYPE ACTIVE,
        print "RX: $(nf_from.count), TX: $(nf_to.n_sent)/$(nf_to.n_dropped)",
        wait 1,
  	  loop
        );
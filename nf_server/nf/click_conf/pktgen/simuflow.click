// Run: sudo click --dpdk -l 0-1 -n 4 -- simuflow.click
define(
    $dev		0,
    $header		"00 00 00 01 00 05",
    $srcmac		0:0:0:0:0:0,
    $dstmac		0:0:0:0:0:0,
    $debug		false,
    $rate		1,
    $flowsize	5000,
    $seed		1,
    $time_offset	4,
    $range		1,
);

P4SFCSimuFlow(
SRCETH $srcmac,
DSTETH $dstmac,
STOP false, DEBUG $debug,
LIMIT -1, RATE $rate, BURST 32,
SRCIP 10.0.0.1, DSTIP 77.77.77.77, RANGE $range, LENGTH 1500,
FLOWSIZE $flowsize,
SFCH \<$header>,
SEED $seed, MAJORFLOW 0.2, MAJORDATA 0.8)
    -> tx::ToDPDKDevice($dev)

rx::FromDPDKDevice($dev)
    -> pt::PrintTime(DEBUG $debug, OFFSET $time_offset)
    -> Discard

Script(
    TYPE ACTIVE,
    print "TX: $(tx.count)/$(tx.dropped); RX: $(rx.count) latency: $(pt.avg_latency)",
    wait 1,
    loop
);

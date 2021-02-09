# P4SFC Network Functions

[NAT](./nf-with-statelib/nat.click),
[Load Balancer](./nf-with-statelib/lb.click),
[firewall](./nf-with-statelib/firewall.click),
[IPS](./nf-with-statelib/ips.click)(based on snort),
[forwarder](./nf-with-statelib/forwarder.click).

## Run Network Functions solely

Though the Network Functions are launched by the server daemon, they can also
be run solely.

- Run pktgen:

```bash
# <pktgen.click> refers to pktgen/*.click
sudo click --dpdk -- <pktgen.click>
```

- Run distributor

```bash
# <distributor.click> refers to pktgen/*.click
sudo click --dpdk -l 0-5 -n 4 --proc-type=primary -v -- distributor.click
```

- Run NFs

```bash
# <nf.click> refers to nf-with-statelib/*.click, nf-without-statelib/*.click
sudo click --dpdk -l 6 -n 4 --proc-type=secondary -v -- <nf.click>
```
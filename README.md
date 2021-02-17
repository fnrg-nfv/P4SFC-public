# P4SFC

A Service Function Chain(SFC) project led by Fudan Future Network Research
Group(FNRG). This project leverages P4 switch to accelerate software-based network
function chains. It offloads proper NFs inside an SFC to reduce the packet
processing latency and increase the throughput for corresponding network
flows.

## System Architecture

From a high-level perspective, P4SFC contains 3 components:

- **Orchestrator**: send the SFC deployment instructions;
- **NF servers**: run the non-offloaded NFs and configure the switch;
- **Programmable (P4) switch**: run the offloaded NF and deliver packets.

The architecture overview of P4SFC is shown below:

![Image of P4SFC Architecture](/etc/overview.png)

## Getting Started

**Physical Environment**: a Tofino-based Switch connected with a server. (The
switch and the server are both in a management network and a fast data
network.)

1. [Configure a server and run a server daemon.](./nf_server/README.md)
1. [Configure a switch and run a switch controller.](./switch/README.md)
1. [Run the orchestrator](./orchestrator/README.md) in the same server for convenience, and send a
request to the orchestrator to deploy an SFC. (You can also configure it in
another server in the same management network.)

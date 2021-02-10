# -*- coding:utf-8 -*-
from flask import Flask, request, jsonify
import requests
import json
import time

# Simplified version that is enough for evaluation!
topo = {"switch 1": "server 1"}

addr_list = {
    "switch 1": "http://10.176.35.32:8090",
    "server 1": "http://10.176.35.14:8090"
}

headers = {'Content-Type': 'application/json'}

nf_offlodability = {
    "L3 Forwader": 0, # fully offloadable
    "Firewall": 0, 
    "NAT": 1, # partially offloadable
    "Load Balancer": 1,
    "IPS": 2 # non-offloadable
}

chain_id = 0
app = Flask(__name__)

def addr2dec(addr):
    items = [int(x) for x in addr.split(".")]
    return sum([items[i] << [24, 16, 8, 0][i] for i in range(4)])


def dec2addr(dec):
    return ".".join([str(dec >> x & 0xff) for x in [24, 16, 8, 0]])


def parse_chain(chain_desc):
    """Parse user input chain
    assign id to each nf and group nf by location
    assgin offloadability to each nf
    """
    global nf_offlodability
    nf_id = 0
    cur_location = None
    cur_group = None
    nf_groups = {}
    for nf in chain_desc:
        # assign id first
        nf['id'] = nf_id
        nf_id = nf_id + 1

        # assgin offloadability
        nf['offloadability'] = nf_offlodability[nf['name']]

        # group by location
        location = nf['location']
        nf_group = nf_groups.get(location, [])
        nf_group.append(nf)
        nf_groups[location] = nf_group

    return nf_groups


def get_connected_server(switch):
    return topo[switch]

def parse_route(chain_route, nf_groups, chain_id, chain_length):
    route_infos = {}
    for switch in chain_route:
        if switch != "egress" and switch != "ingress":
            server = get_connected_server(switch)
            num_nfs = len(
                nf_groups.get(server)) if nf_groups.get(server) != None else 0
            chain_length = chain_length - num_nfs
            route_infos[switch] = {
                "chain_id": chain_id,
                "chain_length": chain_length,
                "output_port": 0
            }
    return route_infos


@app.route('/test')
def test():
    return "Hello from p4sfc ochestrator\n"


@app.route('/deploy_chain', methods=['POST'])
def deploy_chain():
    receive_time = int(time.time() * 1000)

    global chain_id
    global server_addr
    global headers

    data = request.get_json()
    chain_desc = data.get("chain_desc")
    chain_length = len(chain_desc)
    nf_groups = parse_chain(chain_desc)

    for server, nfs in nf_groups.iteritems():
        url = addr_list[server] + "/deploy_chain"
        payload = {
            "chain_id": chain_id,
            "chain_length": chain_length,
            "nfs": nfs
        }
        chain_length = chain_length - len(nfs)
        requests.request("POST",
                         url,
                         headers=headers,
                         data=json.dumps(payload))

    chain_route = data.get("route")
    chain_length = len(chain_desc)
    route_infos = parse_route(chain_route, nf_groups, chain_id, chain_length)
    complete_time = 0
    for switch, route_info in route_infos.iteritems():
        url = addr_list[switch] + "/config_route"
        payload = {
            "type": "insert",
            "chain_id": route_info["chain_id"],
            "chain_length": route_info["chain_length"],
            "output_port": route_info["output_port"]
        }
        response = requests.request("POST",
                                    url,
                                    headers=headers,
                                    data=json.dumps(payload))
        complete_time = max(int(response.text), complete_time)

    response_payload = {
        "chain_id": chain_id,
        "receive_time": receive_time,
        "complete_time": complete_time
    }

    chain_id = chain_id + 1
    return jsonify(response_payload)


if __name__ == '__main__':
    app.run(host="0.0.0.0", port='8092')
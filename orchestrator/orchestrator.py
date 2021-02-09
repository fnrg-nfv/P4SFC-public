# -*- coding:utf-8 -*-
from flask import Flask, request, jsonify
import requests
import json
import time
import os
import sys

# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '../utils/'))
import const

# 理论上，在orchestrator中应该维护这样一个全局网络信息
# 用户的请求只会说明希望把什么网络功能运行在什么server上
# 以及这个sfc的route是什么，即会流过哪些交换机
# 然后orchestrator就可以根据用户的信息以及拓扑信息解析出
# 需要的路由信息并配置到交换机上。
# 这个工作繁琐且没有难度，只是需要大量的时间，所以暂时不写！
# network = {
#     "servers": {
#         "server 1": {
#             "daemon_addr": "http://10.149.252.25:8090"
#         },
#         "server 2": {
#             "daemon_addr": "http://10.149.252.24:8090"
#         },
#         .....
#     },
#     "switches": {
#         "ingress": {
#             "control_plane_addr": "http://10.149.252.20:8090"
#         },
#         "switch 1": {
#             "control_plane_addr": "http://10.149.252.66:8090"
#         },
#         ...
#     }
#     "linkd": {
#         "switch 1-port 1-ingress-port-2",
#         "switch 1-port 2-server 1",
#         "switch 1-port 3-server 2",
#         ...
#     }
# }


def addr2dec(addr):
    "将点分十进制IP地址转换成十进制整数"
    items = [int(x) for x in addr.split(".")]
    return sum([items[i] << [24, 16, 8, 0][i] for i in range(4)])


def dec2addr(dec):
    "将十进制整数IP转换成点分十进制的字符串IP地址"
    return ".".join([str(dec >> x & 0xff) for x in [24, 16, 8, 0]])


app = Flask(__name__)

chain_id = 0

# below is very simplified version,
# but is enough for evaluation!
topo = {"switch 1": "server 1"}

addr_list = {
    "switch 1": "http://10.176.35.32:8090",
    "server 1": "http://localhost:8091"
}

headers = {'Content-Type': 'application/json'}

nf_offlodability = {
    "Monitor": const.OFFLOADABLE,
    "Firewall": const.OFFLOADABLE,
    "IPRewriter": const.PARTIAL_OFFLOADABLE,
    "IPS": const.UN_OFFLOADABLE
}


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
    """Very simplified version !
    Assuming that a switch only connects to one server
    """
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
                "output_port": 0  # 硬编码一下，本来应该根据拓扑决定
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

    # user request example
    app.run(host="0.0.0.0", port='8092', debug=True)

    # User request example
    {
        "chain_desc": [{
            "name": "Monitor",
            "click_config": {
                "param1": "abc"
            },
            "location": "server 1"
        }, {
            "name": "Firewall",
            "click_config": {
                "param1": "abc"
            },
            "location": "server 1"
        }, {
            "name": "IPS",
            "location": "server 1"
        }, {
            "name": "IPRewriter",
            "location": "server 1"
        }],
        "route": ["ingress", "switch 1", "egress"]
    }

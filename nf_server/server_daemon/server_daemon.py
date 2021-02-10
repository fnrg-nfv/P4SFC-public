from flask import Flask, request, jsonify
import requests
import json
import argparse
import time
from pipeline_entry_generator import SFC, generate_entries
from click_nf_runner import start_nfs
import const

app = Flask(__name__)

headers = {
    'Content-Type': 'application/json'
}

switch_controller_addr = "http://10.176.35.32:8090"

@app.route('/test')
def test():
    return "Hello from server deamon"


@app.route('/deploy_chain', methods=["POST"])
def deploy_chain():
    data = request.get_json()
    chain_id = data.get("chain_id")
    chain_length = data.get("chain_length")
    nfs = data.get("nfs")

    # construct sfc
    sfc = SFC(chain_id, chain_length, nfs)

    # start nfs in the server
    start_nfs(sfc.chain_head, chain_id)

    # get pkt process logic rules
    entry_infos = generate_entries(sfc)

    # send to switch controller
    payload = {
        "type": "insert",
        "entry_infos": entry_infos
    }
    requests.request("POST", switch_controller_addr + "/config_pipeline", headers=headers,
                         data=json.dumps(payload))
    return "OK"

def main(server_port):
    print 'P4SFC server daemon init successfully...'
    app.run(host="0.0.0.0", port=server_port)
    


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='P4SFC Server Daemon')
    parser.add_argument('--server-port',
                        help='port for RESTful API',
                        type=str,
                        action="store",
                        required=False,
                        default=8091)
    args = parser.parse_args()
    main(args.server_port)
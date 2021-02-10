from flask import Flask, request
import argparse
import time
import threading
from p4_controller import P4Controller
from state_allocator import EntryManager

app = Flask(__name__)
p4_controller = None
entry_manager = None


@app.route('/test')
def test():
    return "Hello from switch control plane controller~!"


@app.route('/config_pipeline', methods=["POST"])
def config_pipeline():
    data = request.get_json()
    operation_type = data.get("type")
    if operation_type == "insert":
        entries = []
        entry_infos = data.get("entry_infos")
        for entry_info in entry_infos:
            entries.append(p4_controller.build_table_entry(entry_info))
        p4_controller.insert_table_entries(entries)

    return "OK"


@app.route("/config_route", methods=["POST"])
def config_route():
    data = request.get_json()
    chain_id = data.get("chain_id")
    chain_length = data.get("chain_length")
    output_port = data.get("output_port")

    operation_type = data.get("type")
    if operation_type == "insert":
        p4_controller.insert_route(chain_id, chain_length, output_port)

    return str(int(time.time() * 1000))


@app.route("/new_nf", methods=["POST"])
def new_nf():
    data = request.get_json()
    ip_addr = data.get("ip_addr")
    port = data.get("port")
    entry_manager.add_grpc_client("%s:%s" % (ip_addr, port))


def main(p4info_file_path, tofino_config_fpath, server_port, max_rule,
         polling_time, enable_topk, debug):
    global p4_controller
    p4_controller = P4Controller(p4info_file_path, tofino_config_fpath, debug)

    if max_rule > 0 or polling_time > 0:
        global entry_manager
        entry_manager = EntryManager(
            p4_controller, [],
            max_rule, polling_time, enable_topk)
        entry_manager.start()

    app.run(host="0.0.0.0", port=server_port)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='P4SFC switch controller')
    parser.add_argument('--p4info',
                        help='p4info proto in text format from p4c',
                        type=str,
                        action="store",
                        required=False,
                        default='../../build/p4info.pb.txt')
    parser.add_argument('--tofino-config',
                        help='Tofino config file from p4c',
                        type=str,
                        action="store",
                        required=False,
                        default='../../build/device_config')
    parser.add_argument('--server-port',
                        help='port for RESTful API',
                        type=str,
                        action="store",
                        required=False,
                        default=8090)
    parser.add_argument('--max-rule',
                        help='The maximum number of rules a switch can hold',
                        type=int,
                        action="store",
                        required=False,
                        default=1000)
    parser.add_argument(
        '--polling-time',
        help='The polling time (in seconds) for poll the entries',
        type=float,
        action="store",
        required=False,
        default=30)

    parser.add_argument('--enable-topk',
                        dest='enable_topk',
                        action='store_true')
    parser.add_argument('--disable-topk',
                        dest='enable_topk',
                        action='store_false')
    parser.add_argument('--debug', dest='debug', action='store_true')
    parser.set_defaults(enable_topk=True)
    parser.set_defaults(debug=False)
    args = parser.parse_args()
    main(args.p4info, args.tofino_config, args.server_port, args.max_rule,
         args.polling_time, args.enable_topk, args.debug)
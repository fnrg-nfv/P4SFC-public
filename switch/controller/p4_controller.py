import argparse
import grpc
from lib.p4runtime_lib import helper
from lib.p4runtime_lib import tofino
from lib.p4runtime_lib.convert import encodeNum


class P4Controller(object):

    def __init__(self, p4info_file_path, tofino_config_fpath, debug=False):
        self.p4info_helper = helper.P4InfoHelper(p4info_file_path)
        self.switch_connection = tofino.TofinoSwitchConnection(
            name='s1',
            address='127.0.0.1:50051',
            device_id=0,
        )
        self.switch_connection.MasterArbitrationUpdate()
        self.switch_connection.SetForwardingPipelineConfig(
            p4info=self.p4info_helper.p4info,
            tofino_config_fpath=tofino_config_fpath)

        self.debug = debug
        self.table_name_2_element_name = {
            "IpRewriter_exact": "ipRewriter",
            "IpFilter_exact": "ipFilter",
            "IpClassifier_exact": "ipClassifier"
        }
        self.action_name_2_element_name = {
            "rewrite": "ipRewriter",
            "allow": "ipFilter",
            "forward": "ipClassifier"
        }

    def __get_chain_id(self, click_id):
        return click_id >> 16

    def __get_nf_id(self, click_id):
        return (click_id >> 8) & 0x000000ff

    def __get_stage_index(self, click_id):
        return click_id & 0x000000ff

    def __get_prefix(self, stage_id):
        return "MyIngress.elementControl_%d" % (stage_id % 5)

    def __get_element_name_by_table_name(self, table_name):
        return self.table_name_2_element_name[table_name]

    def __get_element_name_by_action_name(self, action_name):
        return self.action_name_2_element_name[action_name]

    def build_table_entry(self, entry_info):
        table_entry = self.p4info_helper.buildTableEntry(
            table_name=entry_info["table_name"],
            match_fields=entry_info["match_fields"],
            action_name=entry_info["action_name"],
            action_params=entry_info.get("action_params", {}),
            priority=entry_info.get("priority", 0))
        return table_entry

    def build_table_entry_with_id(self, click_id, entry_info):
        chain_id = self.__get_chain_id(click_id)
        nf_id = self.__get_nf_id(click_id)
        stage = self.__get_stage_index(click_id)

        table_name = entry_info["table_name"]
        full_table_name = "%s.%s.%s" % (
            self.__get_prefix(stage),
            self.__get_element_name_by_table_name(table_name), table_name)
        entry_info["table_name"] = full_table_name

        action_name = entry_info["action_name"]
        full_action_name = "%s.%s.%s" % (
            self.__get_prefix(stage),
            self.__get_element_name_by_action_name(action_name), action_name)
        entry_info["action_name"] = full_action_name

        match_fields = entry_info["match_fields"]
        match_fields["hdr.sfc.chainId"] = encodeNum(chain_id, 16)
        match_fields["meta.curNfInstanceId"] = encodeNum(nf_id, 15)
        match_fields['meta.stageId'] = encodeNum(0, 8)
        return self.build_table_entry(entry_info)

    def insert_table_entries(self, entries):
        for entry in entries:
            self.switch_connection.WriteTableEntry(entry)

    def insert_table_entry(self, entry):
        if self.debug:
            print "Entry insert...."
            self.showTableEntry(entry)
        try:
            self.switch_connection.WriteTableEntry(entry)
            return 0
        except:
            print "Rule already exist..."
            return -1

    def delete_table_entry(self, entry):
        if self.debug:
            print "Entry delete...."
            self.showTableEntry(entry)
        self.switch_connection.DeleteTableEntry(entry)

    def read_direct_counters(self, table_entries):
        ret = []
        for response in self.switch_connection.ReadDirectCounters(
                table_entries=table_entries):
            for entity in response.entities:
                ret.append(entity.direct_counter_entry.data.packet_count)

        return ret

    def reset_direct_counter(self, table_entry):
        self.switch_connection.ResetDirectCounters(table_entry)

    def showTableEntry(self, entry):
        table_name = self.p4info_helper.get_tables_name(entry.table_id)
        print table_name
        for m in entry.match:
            print "    %s: %r" % (self.p4info_helper.get_match_field_name(
                table_name,
                m.field_id), self.p4info_helper.get_match_field_value(m))
        action = entry.action.action
        action_name = self.p4info_helper.get_actions_name(action.action_id)
        print '        ->', action_name
        for p in action.params:
            print "            %s: %r" % (
                self.p4info_helper.get_action_param_name(
                    action_name, p.param_id), p.value)
        print

    def insert_route(self, chain_id, chain_length, output_port):
        table_entry = self.p4info_helper.buildTableEntry(
            table_name="MyIngress.forwardControl.chainId_exact",
            match_fields={
                "hdr.sfc.chainId": chain_id,
                "hdr.sfc.chainLength": chain_length,
            },
            action_name="MyIngress.forwardControl.set_output_port",
            action_params={"port": output_port})
        self.insert_table_entries([table_entry])


if __name__ == '__main__':
    print "Hello from p4 controller module...."

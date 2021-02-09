import os
import sys

# Import P4Runtime lib from parent utils dir
# Probably there's a better way of doing this.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 '../utils/'))
import const


free_switch_port = [{'port_id': 1, 'vnic_name': 'veth1'}, {'port_id': 2, 'vnic_name': 'veth2'}, {'port_id': 3, 'vnic_name': 'veth3'}, {'port_id': 4, 'vnic_name': 'veth4'}, {'port_id': 5, 'vnic_name': 'veth5'}, {'port_id': 6, 'vnic_name': 'veth6'}, {'port_id': 7, 'vnic_name': 'veth7'}, {'port_id': 8, 'vnic_name': 'veth8'}, {'port_id': 9, 'vnic_name': 'veth9'}, {'port_id': 10, 'vnic_name': 'veth10'}, {'port_id': 11, 'vnic_name': 'veth11'}, {'port_id': 12, 'vnic_name': 'veth12'}, {'port_id': 13, 'vnic_name': 'veth13'}, {'port_id': 14, 'vnic_name': 'veth14'}, {'port_id': 15, 'vnic_name': 'veth15'}, {'port_id': 16, 'vnic_name': 'veth16'}, {'port_id': 17, 'vnic_name': 'veth17'}, {'port_id': 18, 'vnic_name': 'veth18'}, {'port_id': 19, 'vnic_name': 'veth19'}, {'port_id': 20, 'vnic_name': 'veth20'}, {'port_id': 21, 'vnic_name': 'veth21'}, {'port_id': 22, 'vnic_name': 'veth22'}, {'port_id': 23, 'vnic_name': 'veth23'}, {'port_id': 24, 'vnic_name': 'veth24'}, {'port_id': 25, 'vnic_name': 'veth25'}, {'port_id': 26, 'vnic_name': 'veth26'}, {'port_id': 27, 'vnic_name': 'veth27'}, {'port_id': 28, 'vnic_name': 'veth28'}, {'port_id': 29, 'vnic_name': 'veth29'}, {'port_id': 30, 'vnic_name': 'veth30'}, {'port_id': 31, 'vnic_name': 'veth31'}, {'port_id': 32, 'vnic_name': 'veth32'}, {'port_id': 33, 'vnic_name': 'veth33'}, {'port_id': 34, 'vnic_name': 'veth34'}, {'port_id': 35, 'vnic_name': 'veth35'}, {'port_id': 36, 'vnic_name': 'veth36'}, {'port_id': 37, 'vnic_name': 'veth37'}, {'port_id': 38, 'vnic_name': 'veth38'}, {'port_id': 39, 'vnic_name': 'veth39'}, {'port_id': 40, 'vnic_name': 'veth40'}, {'port_id': 41, 'vnic_name': 'veth41'}, {'port_id': 42, 'vnic_name': 'veth42'}, {'port_id': 43, 'vnic_name': 'veth43'}, {'port_id': 44, 'vnic_name': 'veth44'}, {'port_id': 45, 'vnic_name': 'veth45'}, {'port_id': 46, 'vnic_name': 'veth46'}, {'port_id': 47, 'vnic_name': 'veth47'}, {'port_id': 48, 'vnic_name': 'veth48'}, {'port_id': 49, 'vnic_name': 'veth49'}, {'port_id': 50, 'vnic_name': 'veth50'}, {
    'port_id': 51, 'vnic_name': 'veth51'}, {'port_id': 52, 'vnic_name': 'veth52'}, {'port_id': 53, 'vnic_name': 'veth53'}, {'port_id': 54, 'vnic_name': 'veth54'}, {'port_id': 55, 'vnic_name': 'veth55'}, {'port_id': 56, 'vnic_name': 'veth56'}, {'port_id': 57, 'vnic_name': 'veth57'}, {'port_id': 58, 'vnic_name': 'veth58'}, {'port_id': 59, 'vnic_name': 'veth59'}, {'port_id': 60, 'vnic_name': 'veth60'}, {'port_id': 61, 'vnic_name': 'veth61'}, {'port_id': 62, 'vnic_name': 'veth62'}, {'port_id': 63, 'vnic_name': 'veth63'}, {'port_id': 64, 'vnic_name': 'veth64'}, {'port_id': 65, 'vnic_name': 'veth65'}, {'port_id': 66, 'vnic_name': 'veth66'}, {'port_id': 67, 'vnic_name': 'veth67'}, {'port_id': 68, 'vnic_name': 'veth68'}, {'port_id': 69, 'vnic_name': 'veth69'}, {'port_id': 70, 'vnic_name': 'veth70'}, {'port_id': 71, 'vnic_name': 'veth71'}, {'port_id': 72, 'vnic_name': 'veth72'}, {'port_id': 73, 'vnic_name': 'veth73'}, {'port_id': 74, 'vnic_name': 'veth74'}, {'port_id': 75, 'vnic_name': 'veth75'}, {'port_id': 76, 'vnic_name': 'veth76'}, {'port_id': 77, 'vnic_name': 'veth77'}, {'port_id': 78, 'vnic_name': 'veth78'}, {'port_id': 79, 'vnic_name': 'veth79'}, {'port_id': 80, 'vnic_name': 'veth80'}, {'port_id': 81, 'vnic_name': 'veth81'}, {'port_id': 82, 'vnic_name': 'veth82'}, {'port_id': 83, 'vnic_name': 'veth83'}, {'port_id': 84, 'vnic_name': 'veth84'}, {'port_id': 85, 'vnic_name': 'veth85'}, {'port_id': 86, 'vnic_name': 'veth86'}, {'port_id': 87, 'vnic_name': 'veth87'}, {'port_id': 88, 'vnic_name': 'veth88'}, {'port_id': 89, 'vnic_name': 'veth89'}, {'port_id': 90, 'vnic_name': 'veth90'}, {'port_id': 91, 'vnic_name': 'veth91'}, {'port_id': 92, 'vnic_name': 'veth92'}, {'port_id': 93, 'vnic_name': 'veth93'}, {'port_id': 94, 'vnic_name': 'veth94'}, {'port_id': 95, 'vnic_name': 'veth95'}, {'port_id': 96, 'vnic_name': 'veth96'}, {'port_id': 97, 'vnic_name': 'veth97'}, {'port_id': 98, 'vnic_name': 'veth98'}, {'port_id': 99, 'vnic_name': 'veth99'}, {'port_id': 100, 'vnic_name': 'veth100'}]


def get_click_instance_id(chain_id, nf_id, stage_index):
    return (chain_id << 16) + (nf_id << 8) + stage_index


def start_click_nf(nf, chain_id):
    global free_switch_port
    switch_port = free_switch_port.pop(0)
    nic_to_bind = switch_port['vnic_name']
    click_id = get_click_instance_id(chain_id, nf.id, nf.stage_index)
    print 'click id give to element is %d\n' % click_id
    start_command = "sudo ../click_nf/pattern/run.py -i %d ../click_nf/pattern/%s.pattern %s >logs/%s.log 2>&1 &" % (
        click_id, nf.click_file_name, nic_to_bind, nf.click_file_name)
    os.system(start_command)
    return switch_port['port_id']


def start_nfs(chain_head, chain_id):
    """All NFs should be started in the server
    due to the rule-centric of our system
    """
    cur_nf = chain_head
    while cur_nf is not None:
        cur_nf.running_port = start_click_nf(cur_nf, chain_id)
        cur_nf = cur_nf.next_nf


if __name__ == '__main__':
    switch_port = free_switch_port.pop(0)
    nic_to_bind = switch_port['vnic_name']
    click_id = get_click_instance_id(0, 2, 2)
    start_command = "sudo ../click_nf/conf/run.py -i %d ../click_nf/conf/%s.pattern %s >logs/%s.log 2>&1 &" % (
        click_id, "nat-p4-encap", "veth1", "nat-p4-encap")
    os.system(start_command)

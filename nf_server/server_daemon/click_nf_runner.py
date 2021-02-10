free_cores = [i for i in range(1, 12)]

def get_click_instance_id(chain_id, nf_id, stage_index):
    return (chain_id << 16) + (nf_id << 8) + stage_index


def start_click_nf(nf, chain_id):
    global free_cores
    free_core = free_cores.pop(0)
    click_id = get_click_instance_id(chain_id, nf.id, nf.stage_index)
    start_command = '''sudo click --dpdk -l %d -n 4 --proc-type=secondary -v 
                        -- ../nf/click_conf/nf-with-statelib/%s nf_id=%d 
                        >logs/%s.log 2>&1 &''' % (
                            free_core, nf.click_file_name, click_id, nf.click_file_name)
    os.system(start_command)


def start_nfs(chain_head, chain_id):
    """All NFs should be started in the server
    due to the rule-centric of our system
    """
    cur_nf = chain_head
    while cur_nf is not None:
        start_click_nf(cur_nf, chain_id)
        cur_nf = cur_nf.next_nf
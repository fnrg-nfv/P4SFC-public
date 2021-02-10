import threading
import time
import grpc
from lib.p4sfcstate_lib import p4sfcstate_pb2
from lib.p4sfcstate_lib import p4sfcstate_pb2_grpc
from threading import Thread
from collections import OrderedDict


class PullThread(Thread):

    def __init__(self, func, args=()):
        super(PullThread, self).__init__()
        self.func = func
        self.args = args

    def run(self):
        self.result = self.func(*self.args)

    def get_result(self):
        return self.result


class NfEntryWrapper(object):

    def __init__(self, entry, entry_key, click_instance_id, window_cur_pos):
        self.entry = entry
        self.entry_key = entry_key
        self.click_instance_id = click_instance_id
        self.window_cur_pos = window_cur_pos


class InstalledEntry(object):

    def __init__(self, built_entry, window, window_cur_pos):
        self.built_entry = built_entry
        self.window = []
        for i in range(len(window.slot)):
            self.window.append(window.slot[window_cur_pos])
            window_cur_pos = (window_cur_pos - 1) % len(window.slot)

class EntryManager(object):

    def __init__(self, p4_controller, element_addrs, max_rule, polling_time,
                enable_topk):
        self.p4_controller = p4_controller

        # maintain all grpc clients
        self.grpc_clients = []
        for element_addr in element_addrs:
            self.grpc_clients.append(self.__create_grpc_client(element_addr))

        self.max_rule = max_rule
        self.polling_time = polling_time
        self.enable_topk = enable_topk

        self.installed_entries = OrderedDict()

    def __create_grpc_client(self, nf_addr):
        channel = grpc.insecure_channel(nf_addr)
        client = p4sfcstate_pb2_grpc.RPCStub(channel)
        return client

    def register_element(self, nf_addr):
        client = self.__create_grpc_client
        self.grpc_clients.append(client)

    def __pull_switch_entries(self):
        installed_entries = self.installed_entries.values()
        built_entries = [entry.built_entry for entry in installed_entries]
        frequencies = self.p4_controller.read_direct_counters(built_entries)

        for i in range(len(installed_entries)):
            frequency = frequencies[i]
            installed_entry = installed_entries[i]
            installed_entry.window.pop()
            frequency = frequency if frequency >= installed_entry.window[
                0] else installed_entry.window[0]
            installed_entry.window.insert(0, frequency)

        installed_entry_keys = self.installed_entries.keys()
        installed_entry_keys.sort(self.__sort_by_frequency, reverse=True)

        return installed_entry_keys

    def __pull_nf_entries(self, grpc_client):
        entry_request = p4sfcstate_pb2.Empty()
        entry_response = grpc_client.GetState(entry_request)

        return entry_response

    def __pull_new_nf_entries(self, grpc_client):
        entry_request = p4sfcstate_pb2.Empty()
        entry_response = grpc_client.GetNewState(entry_request)

        return entry_response


    def __select_first_k_entries(self, installed_entry_keys, nf_responses):
        nf_entry_sum = 0
        nf_entry_count = 0
        for nf_response in nf_responses:
            nf_entry_sum += len(nf_response.entries)

        nf_indexs = [0] * len(nf_responses)

        num_remain = self.max_rule - len(self.installed_entries)
        num_delete = 0
        if num_remain < nf_entry_sum:
            num_delete = nf_entry_sum - num_remain
            num_delete = min(num_delete, len(self.installed_entries))

        delete_entry_keys = []
        for key in self.installed_entries.keys():
            if num_delete == 0:
                break
            delete_entry_keys.append(key)
            num_delete -= 1

        k = min(self.max_rule, nf_entry_sum)
        insert_entries = []
        nf_index = 0
        while k > 0 and nf_entry_count < nf_entry_sum:
            if nf_indexs[nf_index] == len(nf_responses[nf_index].entries):
                nf_index += 1
            else:
                entry_key = ''
                for match in nf_responses[nf_index].entries[
                        nf_indexs[nf_index]].match:
                    entry_key += match.exact.value

                # only insert new entry
                if entry_key not in self.installed_entries:
                    k -= 1
                    insert_entries.append(
                        NfEntryWrapper(
                            nf_responses[nf_index].entries[nf_indexs[nf_index]],
                            entry_key, nf_responses[nf_index].click_instance_id,
                            nf_responses[nf_index].window_cur_pos))

                nf_entry_count += 1
                nf_indexs[nf_index] += 1
        return delete_entry_keys, insert_entries

    def __select_top_k_entries(self, installed_entry_keys, nf_responses):
        nf_entry_sum = 0
        nf_entry_count = 0
        for nf_response in nf_responses:
            nf_entry_sum += len(nf_response.entries)

        installed_index = 0
        nf_indexs = [0] * len(nf_responses)

        k = self.max_rule
        insert_entries = []
        while k > 0 and (installed_index < len(installed_entry_keys) or
                         nf_entry_count < nf_entry_sum):
            if nf_entry_count == nf_entry_sum:
                installed_index = min(len(installed_entry_keys),
                                      installed_index + k)
                break
            else:
                # select most frequent nf entry
                max_frequency = -1
                nf_index = -1
                for i in range(len(nf_indexs)):
                    if nf_indexs[i] < len(nf_responses[i].entries):
                        entry = nf_responses[i].entries[nf_indexs[i]]
                        window_next_pos = (nf_responses[i].window_cur_pos +
                                           1) % len(entry.window.slot)
                        frequency = entry.window.slot[
                            nf_responses[i].
                            window_cur_pos] - entry.window.slot[window_next_pos]

                        if frequency > max_frequency:
                            max_frequency = frequency
                            nf_index = i

                if installed_index < len(
                        installed_entry_keys) and max_frequency <= (
                            self.installed_entries[
                                installed_entry_keys[installed_index]].window[0]
                            - self.installed_entries[installed_entry_keys[
                                installed_index]].window[-1]):
                    installed_index += 1
                    k -= 1
                else:
                    # need entry duplicate check
                    entry_key = str(nf_responses[nf_index].click_instance_id)
                    for match in nf_responses[nf_index].entries[
                            nf_indexs[nf_index]].match:
                        entry_key += match.exact.value

                    # only insert new entry
                    if entry_key not in self.installed_entries:
                        k -= 1
                        insert_entries.append(
                            NfEntryWrapper(
                                nf_responses[nf_index].entries[
                                    nf_indexs[nf_index]], entry_key,
                                nf_responses[nf_index].click_instance_id,
                                nf_responses[nf_index].window_cur_pos))

                    nf_entry_count += 1
                    nf_indexs[nf_index] += 1
        delete_entry_keys = installed_entry_keys[installed_index:]
        print nf_indexs
        return delete_entry_keys, insert_entries

    def __parse_candidate_entry(self, candidate_entry):
        entry_info = {}
        entry_info["table_name"] = candidate_entry.table_name
        match_fields = {}
        for match in candidate_entry.match:
            match_fields[match.field_name] = match.exact.value

        entry_info["match_fields"] = match_fields
        entry_info["action_name"] = candidate_entry.action.action
        action_params = {}
        for param in candidate_entry.action.params:
            action_params[param.param] = param.value

        entry_info["action_params"] = action_params
        return entry_info

    def __sort_by_frequency(self, entry_key1, entry_key2):
        entry1 = self.installed_entries[entry_key1]
        entry2 = self.installed_entries[entry_key2]
        entry1_frequency = entry1.window[0] - entry1.window[-1]
        entry2_frequency = entry2.window[0] - entry2.window[-1]
        if entry1_frequency > entry2_frequency:
            return 1
        elif entry1_frequency < entry2_frequency:
            return -1
        else:
            return 0

    def __update_entries(self):
        print "\n\nupdate entries start..."

        start_time = time.time()

        switch_pull_thread = PullThread(self.__pull_switch_entries)
        switch_pull_thread.start()

        # collect all candidate entries
        nf_pull_threads = []
        for grpc_client in self.grpc_clients:
            nf_pull_thread = None
            if self.enable_topk:
                nf_pull_thread = PullThread(self.__pull_nf_entries, (grpc_client,))
            else:
                nf_pull_thread = PullThread(self.__pull_new_nf_entries, (grpc_client,))

            nf_pull_thread.start()
            nf_pull_threads.append(nf_pull_thread)

        switch_pull_thread.join()
        for thread in nf_pull_threads:
            thread.join()

        installed_entry_keys = switch_pull_thread.get_result()

        nf_responses = []
        for thread in nf_pull_threads:
            nf_response = thread.get_result()
            nf_responses.append(nf_response)

        end_time = time.time()
        print "Time of pull entries: %.3fs" % (end_time - start_time)

        start_time = time.time()
        if self.enable_topk:
            delete_entry_keys, insert_entries = self.__select_top_k_entries(
                installed_entry_keys, nf_responses)
        else:
            delete_entry_keys, insert_entries = self.__select_first_k_entries(
                installed_entry_keys, nf_responses)

        print "delete %d entries..." % (len(delete_entry_keys))
        end_time = time.time()
        print "Time of select k entries: %.3fs" % (end_time - start_time)

        start_time = time.time()
        for key in delete_entry_keys:
            delete_entry = self.installed_entries.pop(key)
            self.p4_controller.delete_table_entry(delete_entry.built_entry)
        end_time = time.time()
        print "Time of delete old entries: %.3fs" % (end_time - start_time)

        start_time = time.time()
        for nfEntryWrapper in insert_entries:
            entry_info = self.__parse_candidate_entry(nfEntryWrapper.entry)
            built_entry = self.p4_controller.build_table_entry_with_id(
                nfEntryWrapper.click_instance_id, entry_info)
            self.p4_controller.insert_table_entry(built_entry)
            self.installed_entries[nfEntryWrapper.entry_key] = InstalledEntry(
                built_entry, nfEntryWrapper.entry.window,
                nfEntryWrapper.window_cur_pos)
        end_time = time.time()
        print "Time of insert new entries: %.3fs" % (end_time - start_time)

        threading.Timer(self.polling_time, self.__update_entries,
                        ()).start()  # start next iteration

    def start(self):
        self.__update_entries()

if __name__ == '__main__':
    print 'Hello from nf entry manager.'
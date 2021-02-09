#include <thread>
#include <grpc++/grpc++.h>

#define DEBUG

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

#include "p4sfcstate.hh"

#include "p4sfcstate.grpc.pb.h"

namespace P4SFCState
{
    using namespace grpc;
    using namespace std;

    vector<TableEntryImpl *> total_entries;
    int cur_pos;
    const int k = 1000;

    // Logic and data behind the server's behavior.
    class ServiceImpl final : public RPC::Service
    {
        Status SayHello(ServerContext *context, const HelloRequest *request,
                        HelloReply *reply) override
        {
            string prefix("Hello ");
            reply->set_message(prefix + request->name());
            return Status::OK;
        }

        Status GetState(ServerContext *context, const Empty *request, TableEntryReply *reply)
        {
#ifdef DEBUG
            auto t_start = chrono::high_resolution_clock::now();
#endif
            reply->set_window_cur_pos(cur_pos);
            reply->set_click_instance_id(_click_instance_id);

            size_t size = total_entries.size();
            for (auto i = total_entries.begin(); i != total_entries.cend(); i++)
            {
                TableEntryImpl *e = *i;
                e->build_slots();
            }

            sort(total_entries.begin(), total_entries.end(),
                 [](const TableEntryImpl *a, const TableEntryImpl *b) {
                     return a->slots_sum > b->slots_sum;
                 });

            size = k < size ? k : size;
            for (size_t i = 0; i < size; i++)
                reply->add_entries()->CopyFrom(*(total_entries[i]));

            move_window_forward();

#ifdef DEBUG
            cout << "Get size:" << size << "/" << total_entries.size() << endl;
            auto t_end = std::chrono::high_resolution_clock::now();
            cout << fixed << setprecision(2)
                 << "Get state time passed:"
                 << chrono::duration<double, milli>(t_end - t_start).count() << " ms\n";
#endif
            return Status::OK;
        }

        Status GetNewState(ServerContext *context, const Empty *request, TableEntryReply *reply)
        {
#ifdef DEBUG
            auto t_start = chrono::high_resolution_clock::now();
#endif
            reply->set_window_cur_pos(cur_pos);
            reply->set_click_instance_id(_click_instance_id);
            static int pos = 0;

            size_t size = total_entries.size();
            if (size > k && pos < size - k)
                pos = size - k;
            for (; pos < size; pos++)
                reply->add_entries()->CopyFrom(*total_entries[pos]);

#ifdef DEBUG
            cout << "Get size:" << reply->entries_size() << "/" << size << endl;
            auto t_end = chrono::high_resolution_clock::now();
            cout << fixed << setprecision(2)
                 << "Get state time passed:"
                 << chrono::duration<double, milli>(t_end - t_start).count() << " ms\n";
#endif
            return Status::OK;
        }

    public:
        ServiceImpl(int click_instance_id) : _click_instance_id(click_instance_id) {}

    private:
        int _click_instance_id;

        void move_window_forward()
        {
            int next_pos = (cur_pos + 1) % WINDOW_SIZE;
            for (auto i = total_entries.begin(); i != total_entries.cend(); i++)
            {
                TableEntryImpl *e = *i;
                e->slots[next_pos] = e->slots[cur_pos];
            }
            cur_pos = next_pos;
        }
    };

    std::unique_ptr<Server> server;
    thread *server_thread;

    void runServer(string addr, int click_instance_id)
    {
        cur_pos = 0;

        ServiceImpl service(click_instance_id);
        ServerBuilder builder;
        builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        server = builder.BuildAndStart();

        cout << "Server listening on " << addr << endl;
        server->Wait();
    }

    void startServer(int click_instance_id, string addr)
    {
        if (server_thread)
            return;
        server_thread = new thread(runServer, addr, click_instance_id);
    }

    void shutdownServer()
    {
        deleteTableEntries();
        server->Shutdown();
        server_thread->join();
    }

    string bigint_to_hexstr(const string &s)
    {
        typedef uint8_t unit;
        const unit *i = (const unit *)s.data();
        stringstream stream;
        stream << "0x";
        int len = s.length() / sizeof(unit);
        for (size_t j = 0; j < len; j++)
        {
            stream << setfill('0') << setw(sizeof(unit) * 2)
                   << hex << unsigned(i[j]);
        }
        return stream.str();
    }

    string TableEntryImpl::unparse() const
    {
        string ret("");
        stringstream stream;
        stream << "TableName: " << table_name();
        stream << "\tMatch: ";
        int size = match_size();
        for (size_t i = 0; i < size; i++)
        {
            auto m = match(i);
            stream << m.field_name() << ": ";
            if (m.has_exact())
                stream << "exact: "
                       << bigint_to_hexstr(m.exact().value());
            else if (m.has_ternary())
                stream << "ternary: "
                       << bigint_to_hexstr(m.ternary().value()) << "/"
                       << bigint_to_hexstr(m.ternary().mask());
            //    << int_to_hexstr(m.ternary().value().data()) << "/"
            //    << int_to_hexstr(m.ternary().mask().data());
            stream << "; ";
        }
        auto a = action();
        size = a.params_size();
        stream << "\tAction: " << a.action() << " param: ";
        for (size_t i = 0; i < size; i++)
        {
            auto p = a.params(i);
            stream << p.param() << ": " << p.value() << "; ";
        }
        stream << "\tPriority: " << to_string(priority());
        auto w = window();
        size = w.slot_size();
        stream << "\tSlidingWindow(" + to_string(size) + "): ";
        for (size_t i = 0; i < size; i++)
            stream << to_string(w.slot(i)) << " ";
        return stream.str();
    }

    TableEntryImpl::TableEntryImpl()
    {
        total_entries.push_back(this);
        // init sliding window
        auto window = mutable_window();
        for (int i = 0; i < WINDOW_SIZE; i++)
            window->add_slot(0);
        std::fill(slots, slots + WINDOW_SIZE, 0);
    }

    void deleteTableEntries()
    {
        for (auto e = total_entries.begin(); e != total_entries.end(); e = total_entries.erase(e))
            if (*e)
                delete *e;
    }

    void TableEntryImpl::build_slots()
    {
        auto w = mutable_window();
        for (size_t i = 0; i < WINDOW_SIZE; i++)
            w->set_slot(i, slots[i]);
        slots_sum = slots[cur_pos] - slots[(cur_pos + 1) % WINDOW_SIZE];
    }
} // namespace P4SFCState
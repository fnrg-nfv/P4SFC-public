
PROTOS_PATH = .
CXX = g++
CPPFLAGS += -I/usr/local/include -pthread -fPIC
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl

PROTOSRC = p4sfcstate.proto

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
GRPC_PY_PLUGIN = grpc_python_plugin
GRPC_PY_PLUGIN_PATH ?= `which $(GRPC_PY_PLUGIN)`


all: libstate.a proto_py

libstate.so: p4sfcstate.pb.o p4sfcstate.grpc.pb.o p4sfcstate.o
	$(CXX) $^ $(LDFLAGS) -shared -o $@

libstate.a: p4sfcstate.pb.o p4sfcstate.grpc.pb.o p4sfcstate.o
	ar -crv $@ $^

client: p4sfcstate.pb.o p4sfcstate.grpc.pb.o p4sfcstate.o client.o
	$(CXX) $^ $(LDFLAGS) -o $@

server: p4sfcstate.pb.o p4sfcstate.grpc.pb.o server.o
	$(CXX) $^ $(LDFLAGS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $^


# .PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

# .PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

proto: p4sfcstate.pb.cc p4sfcstate.grpc.pb.cc 

%_pb2_grpc.py: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --python_out=. --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_PY_PLUGIN_PATH) $<

proto_py: p4sfcstate_pb2_grpc.py

clean:
	rm -f *.o *.so *.pb.cc *.pb.h *_pb2_grpc.py *_pb2.py client server test

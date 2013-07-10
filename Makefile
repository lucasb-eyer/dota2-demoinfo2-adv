EXECUTABLE=demoinfo2

CPP_FILES=$(wildcard *.cpp)
HDR_FILES=$(CPP_FILES:.cpp=.h)
OBJ_FILES=$(CPP_FILES:.cpp=.o)
PROTO_SRC_FILES=$(wildcard *.proto)
PROTO_CPP_FILES=$(addprefix generated_proto/,$(PROTO_SRC_FILES:.proto=.pb.cc))
PROTO_OBJ_FILES=$(PROTO_CPP_FILES:.cc=.o)

CXX?=g++
PROTOC?=protoc
LD_FLAGS=-lsnappy -lprotobuf -lpthread -L/usr/lib
CXX_FLAGS=-I/usr/include -L/usr/lib -DOUTPUT_GameEvent -DOUTPUT_ChatEvent
ROTOBUF_FLAGS=-I/usr/include

all: ${EXECUTABLE}

clean:
	rm -f ${EXECUTABLE}
	rm -f *.o
	rm -f generated_proto/*

generated_proto/%.pb.cc: %.proto
	${PROTOC} ${PROTO_SRC_FILES} ${PROTOBUF_FLAGS} -I. -I/usr/include --cpp_out=generated_proto

${EXECUTABLE}: ${PROTO_OBJ_FILES} ${OBJ_FILES}
	${CXX} ${LD_FLAGS} -o $@ ${OBJ_FILES} ${PROTO_OBJ_FILES}

.cpp.o: ${CPP_FILES} ${HDR_FILES}
	${CXX} ${CXX_FLAGS} -c -o $@ $<

.cc.o: ${PROTO_CPP_FILES}
	${CXX} ${CXX_FLAGS} -c -o $@ $<

.PHONY: all clean

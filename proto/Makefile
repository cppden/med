all: med.pb.h

med.pb.h:	med.proto
	protoc --cpp_out=./ $<

clean:
	rm med.pb.*


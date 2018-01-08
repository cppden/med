# 3. Physical layer
This layer deals with the physical representation of the protocol data.
Due to layered structure of the library the physical layer has no dependency on the layers above thus allowing to minimize the number of elements it needs to handle. The latter drastically simplifies definition of different encoders and decoders.
As of now there is only octet encoder and decoder defined which covers most existing protocols used.
There is also one special type of encoder supported by the library - printer. Its purpose is to dump decoded message contents into a human readable form.
Yet again the regular user of the library doesn't need to know the details of this layer except the fact that there are only octet encoder and decoder currently defined and all elements of messages should have size multiple of 8 bits. Note that even though many protocols have bit fields they are not really bit-oriented as long as they are byte aligned (for example, RADIUS, DIAMETER, GTPC, NAS protocols are best processed with octet encoder/decoder).

## 3.1. Octet encoder/decoder
Also note that only sizes multiple of 8 bits (an octet) can be used with octet encoder and decoder.

## 3.2. Printer

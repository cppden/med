# MED
> Baby, did you forget to take your meds?
> *(c) Placebo*

The Meta-Encoder/Decoder (`med`) is a header only library to define non-ASN.1 messages with independent invocation of required encoder, decoder or printer that are automatically generated via meta-programming.

The primary goal of the library design is to provide ultimate run-time performance on the par with hand-written code yet allowing to easily define complex messages taking care off the user for protocol validation.

The next goal is to mimic ASN.1 design where message definition is abstracted from its physical representation.

Because of performance requirements the library uses no dynamic memory allocation and exceptions for error reporting.
Due to the fact that the library is based on templates any real protocol defined with it will put a noticeable load on the C++ compiler increasing the compilation times. To mitigate this it's advised to keep user code instantiating encoder/decoder in separate files. The latter also helps to mitigate possible code-bloat.

The user of the library needs only to define the structure of the messages consisting particular protocol. All the checks for presence of mandatory and conditional fields and their validation will be performed automatically during decoding and even during encoding by the generated code. When message fails to decode it's still possible to print a part of it which is useful for diagnostic purposes.

The library is split in 3 layers:

1. The [representation layer](Representation-Layer.md) to describe a message from the collection of building blocks, their presence and inter-dependencies if any.

2. The [structural layer](Structural-Layer.md) to provide internal means to traverse constructed protocol, messages and fields.

3. The [physical layer](Physical-Layer.md) to encode or decode the underlying primitives.

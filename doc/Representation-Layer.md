# 1. Representation layer
This layer is a front-end for library user. It consists of three types of building blocks - values, containers and presence definitions of values within a container. A container in its turn can be included as an aggregate value in another container thus constructing complex information elements.

## 1.1. Values
### 1.1.1. Generic integer
Any message field that is of an integer type should be represented using med::value template class. The only template parameter it needs is the size of the integer field in bits. Only unsigned integers are supported with maximum size of 64 bits.

A typical definition of a field looks like this:
```cpp
struct afield : med::value<24>{
  static constexpr char const* name() { return "Field A"; }
};
```
The definition of the field name is optional and only needed if you use med::printer and want this field to be printed.
If the field has some restriction on its value you need to define the set member function returning bool as follows:
```cpp
bool set(value_type v) {
  if (condition(v)){
    base_t::set(v);
    return true;
  }
  return false;
}
```
Note that you shouldn't use exceptions since the library is not designed this way because of performance reasons while error context is captured automatically  and can be extracted later after encoding or decoding and used to throw an exception if needed.
CAUTION: Do not override the get member functions to force any restrictions!

### 1.1.2. Octet string
This value type represents a sequence of octets, i.e. an array or a string. There are 2 options for data storage in octet string - internal or external. The internal storage makes it possible to easily copy the field but impose a performance penalty both on CPU and memory usage. Thus it's only recommended for small string of complex structure. The external storage is the best option for lengthy strings as it only holds a pointer to the data and its size. Therefore it requires special care of the pointed data buffer and its lifetime relative to the lifetime of the message itself and is not suitable for copying of the field (see also the note on encoder/decoder features for a possible solution).
The template of the octet string accepts up to 3 parameters to specify restrictions on minimum and/or maximal length, varying or fixed size and the storage type discussed.

## 1.2. Containers
The containers are the means to build a message from information elements and a protocol from the messages as well as to define complex IEs.

### 1.2.1. Sequence
This type of container represents a sequence of elements which follow in strict order one after another. It is the most common way to define a message and unlike the rest of containers it doesn't put any requirements on its elements.
Example...

### 1.2.2. Set
The set container is similar to the sequence container as it represents a number of elements. But elements of the set can appear in any order in encoded stream. To achieve this every element in the set must not only to have some identity but moreover the "structure" of their identity should be the same, that is every set element should have a common header. Such a header can be an integer tag or some sequence which knows how to form a tag equivalent (e.g. radius AVP header).

### 1.2.3. Choice
Just like the set container the choice requires its elements to have a common header but only one element can appear in encoded stream. This type of container can always be found on the top level of any protocol.

### 1.2.4. Accessors
The accessors of elements in containers are naturally divided in 2 categories - for encoding，read-write (ref), and for decoding, read-only (get). These can be treated as explicit accessors since require explicit specification of the accessed field:
```cpp
nas::esm_plain const& esm = msg->get<nas::esm_container>().get<nas::esm_plain>();
```
There are another implicit accessors (`field` in `sequence` and `set`, `select` in `choice`) for for usability purposes which deduce the accessed field type from lvalue:
```cpp
nas::esm_plain const& esm = msg->get<nas::esm_container>().select();
```

The important thing to remember is that read-write accessors return reference of the field while read-only accessors return const reference for mandatory but const pointer for optional fields to reflect that optional field may not present in the decoded message.

## 1.3. Presence definitions
This part of the library serves as a glue between elements and containers holding them. Besides specification of the presence itself it also allows to specify the arity of the element and its conditional dependencies if any.

### 1.3.1. Mandatory
The `med::mandatory` accepts from 1 to 3 template arguments specifying the element's value itself, its tag and/or length, and optionally its arity (by default the arity of mandatory element is one).

### 1.3.2. Optional
The `med::optional` is similar to the mandatory presence definition but requires a tag, a length, or a condition to define its presence. The default arity of the optional element is zero.

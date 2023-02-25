# Most important
## All IEs as value types
use offsets instead of pointers to reduce memory footprint and allow simple copy
## Review code to reduce code-bloat
* Remove excessive template parameters
* Common part of buffer via type-erasure + trampolines
* Non-templated allocator
* Introduce .cpp files?

# Medium importance
## More concepts to improve errors

# Minor importance
## Independent decode/encode in UTs
it's much simpler to debug encode independent of decode

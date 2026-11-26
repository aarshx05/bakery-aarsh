# Fuzzing Build Instructions

## Prerequisites
- GCC 13+ with ASAN/UBSAN support
- OpenSSL development headers (`libssl-dev`)
- zlib development headers (`zlib1g-dev`)
- tgcalls source cloned alongside this directory

## Build Commands

All commands assume `$TGCALLS` points to the tgcalls source root (containing the `tgcalls/` subdirectory) and `$STUBS` points to the `stubs/` directory here.

### Common flags
```bash
CXXFLAGS="-std=c++20 -fsanitize=address,undefined -g -O1 -include $STUBS/force_include.h -I $STUBS -I $TGCALLS/tgcalls"
```

### harness_reflector
```bash
g++ -std=c++17 -fsanitize=address,undefined -g -O1 \
    harness_reflector.cpp -o harness_reflector
```

### harness_gzip
```bash
g++ -std=c++17 -fsanitize=address,undefined -g -O1 \
    harness_gzip.cpp $TGCALLS/tgcalls/utils/gzip.cpp \
    -I $TGCALLS/tgcalls -lz -o harness_gzip
```

### harness_message
```bash
g++ $CXXFLAGS \
    harness_message.cpp $TGCALLS/tgcalls/Message.cpp \
    -o harness_message
```

### harness_group_frame
```bash
g++ $CXXFLAGS \
    harness_group_frame.cpp $TGCALLS/tgcalls/group/GroupFrameTransformer.cpp \
    -o harness_group_frame
```

### harness_encrypted
```bash
g++ $CXXFLAGS -iquote $STUBS -Wno-deprecated-declarations \
    harness_encrypted.cpp \
    $TGCALLS/tgcalls/EncryptedConnection.cpp \
    $TGCALLS/tgcalls/CryptoHelper.cpp \
    $TGCALLS/tgcalls/Message.cpp \
    -lssl -lcrypto -o harness_encrypted
```

### harness_signaling
```bash
g++ $CXXFLAGS \
    harness_signaling.cpp $TGCALLS/tgcalls/third-party/json11.cpp \
    -I $TGCALLS/tgcalls -o harness_signaling
```

## Running

### Seed corpus
```bash
./harness_<name> corpus/<name>/*.bin
```

### Mutation fuzzing
```bash
bash mutate_and_fuzz.sh ./harness_<name> corpus/<name> 2000
```

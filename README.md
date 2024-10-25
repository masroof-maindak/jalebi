# Jalebi

### TODO

- [ ] Server-side authentication
    - [x] thread-local uid & userDir
    - [x] `HOSTDIR` -> `HOSTDIR/userid`
    - [ ] SQLite integration
- [ ] queue.h/.c
    - [ ] server-side handler thread
    - [ ] reader/writer threads depending on uid
- [ ] Use restrict keyword where applicable
- [ ] close() -> shutdown() for sockets; should I?
    - [ ] Relevant signal handlers
- [ ] Log error messages in non-perror-able call stacks
- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic

# Jalebi

### TODO

- [/] auth.h/.c - login/register
    - [x] SQLite integration
    - [/] thread-local userid
    - [ ] `HOSTDIR` -> `HOSTDIR/userid`
- [ ] queue.h/.c
    - [ ] server-side handler thread
    - [ ] reader/writer threads depending on userid
- [ ] Use restrict keywords where applicable
- [ ] close() -> shutdown() for sockets; should I?
    - [ ] Relevant signal handlers
- [ ] Log error messages in non-perror-able call stacks
- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic

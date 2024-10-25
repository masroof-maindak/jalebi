# Jalebi

### TODO

- [ ] Server-side authentication
    - [x] thread-local uid & userDir
    - [x] `HOSTDIR` -> `HOSTDIR/userid`
    - [ ] SQLite integration
- [ ] queue.h/.c
    - [ ] server-side handler thread
    - [ ] reader/writer threads depending on uid?
---
- [ ] close() -> shutdown() for sockets ???
- [ ] Log error messages in non-perror-able 'dead ends'
- [ ] Signal handlers -> sockets/readline(?)
- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic
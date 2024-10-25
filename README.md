# Jalebi

### TODO

- [ ] Server-side authentication
    - [x] thread-local uid & userDir
    - [x] `HOSTDIR` -> `HOSTDIR/userid`
    - [ ] SQLite integration
- [ ] server-side file handler thread
- [ ] reader/writer threads depending on uid?

---

- [ ] close() -> shutdown() for sockets ???
- [ ] Signal handlers; deal w/ sockets/readline(?)
- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic

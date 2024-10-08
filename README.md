# Jalebi

### TODO
- [x] Client-side download
- [x] fix `strtok` in client.c
- [/] auth.h/.c - login/register
    - [x] SQLite integration
    - [ ] thread-local userid
    - [ ] Replace all uses of `HOSTDIR` w/ `HOSTDIR/userid`
- [ ] queue.h/.c
    - [ ] server-side handler thread
    - [ ] reader/writer threads depending on userid
- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic
- [ ] Use an ENUM for `reqType`
- [ ] Get rid of readline
- [ ] Use the [ restrict ] keyword where suitable

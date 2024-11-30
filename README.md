# Jalebi

### Dependencies

- GNU Readline
- libbsd
- SQLite3
- OpenSSL

#### Arch

```bash
sudo pacman -S lib32-sqlite lib32-openssl readline libbsd util-linux
```

#### Debian

```bash
sudo apt install libsqlite3-dev libssl-dev libreadline-dev libbsd-dev uuid-dev
```

### Todo

- [x] Threadpool -> incorporate queue where arbitrary tasks are pushed to
- [x] Worker thread entrypoint
    - [ ] Global session management - hashmap
    - [x] Process task
    - [x] Mark answer in hashmap & signal communication thread

---

- [ ] Refactor struct answer to simply be an enum status (the uuid\_t in the struct is unnecessary)
- [ ] Thread monitoring - Restart killed threads, etc.
    - How to find out a thread's died?
- [ ] close() -> shutdown() for sockets ?
- [ ] Signal handlers; gracefully deal w/ sockets/readline(?)/database/threadpool/etc.

---

- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic

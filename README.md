# Jalebi

### Dependencies

- GNU Readline
- libbsd
- SQLite3
- OpenSSL

#### Arch

```bash
sudo pacman -S lib32-sqlite lib32-openssl readline libbsd
```

#### Debian

```bash
sudo apt install libsqlite3-dev libssl-dev libreadline-dev libbsd-dev
```

### Todo

- [ ] Producer-consumer problem?
- [ ] Threadpool -> incorporate queue
- [ ] Worker thread function
    - [ ] Worker queue
    - [ ] Global session management
        - [ ] Dynamic mutex array?
    - [ ] Process task (easy)
    - [ ] Push answer to **answer queue**

---

- [ ] Thread monitoring - Restart killed threads, etc.
    - How to find out a thread's died?
- [ ] close() -> shutdown() for sockets ???
- [ ] Signal handlers; gracefully deal w/ sockets/readline(?)/database/threadpool (!!!)

---

- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic

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

### Todo

- [ ] Server: threadpool w/ queue

---

- [ ] close() -> shutdown() for sockets ???
- [ ] Signal handlers; gracefully deal w/ sockets/readline(?)/database/threadpool (most important)

---

- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic

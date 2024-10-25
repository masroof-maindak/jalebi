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

- [ ] Auth: register\_user should return the UID
- [ ] Server: authentication; SQLite integration
- [ ] Server: file handling thread
    - [ ] Thread queue depending on UID???

---

- [ ] close() -> shutdown() for sockets ???
- [ ] Signal handlers; gracefully deal w/ sockets/readline(?)/ database
- [ ] encoding/decoding
    - [ ] use an algorithm that guarantees a smaller result
    - [ ] function pointer-ed upload and download logic

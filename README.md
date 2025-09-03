# Jalebi

Jalebi (+ Namak Paare) is a peculiar little server-client application set responsible for creating an authenticated 'cloud-based' storage environemnt. I'm quite content with its current state and would consider it my first major C program -- one that taught me how to truly use/vastly enhanced my knowledge of an array of tools including but not limited to `TSan`, `Valgrind`, `gdb`, `perf` and most importantly, `man` pages.

> It has also taught me how much I fucking hate *not* having a fixed, idiomatic way to handle errors, for instance Rust's Result types, or Go's error types.

Both binaries, but the server in particular, are to the best of my juvenile knowledge, quite robust in terms of error-handling, catering to imperfect input, and tiny nitpicks pertaining to the usage of certain syscalls/libc functions and ANSI-C compliance -- the lattermost notably requiring arcane knowledge uncovered by poring through tomes inscribed by wise sages of the past (i.e, reading manpages).

In hindsight, there are a handful of things I could have improved upon, for instance 'typing' erroneous return values via enums, using the `sendfile()` syscall for some spicy zero-copy action via DMA (I don't regret doing it the 'manual way' my considering this was my first **proper** plunge into C sockets), minorly-incomplete clean-exiting for the threadpool, or some rather trivial `TODO`s, but I am choosing to leave this project as is to serve as a keepsake of days (and skill levels) bygone.

Furthermore, I _am_ quite proud of certain things, for instance the novel architecture, profoundly graceful `cond_var_t` usage (or so I would like to delude myself), from-scratch, generic threadpool & queue implementations (albeit customary for C programmers, these provided a nice respite from wanting to bash my head against the wall whilst `x/`-ing my way through `gdb`).

## Architecture

![architecture](.github/assets/Jalebi.drawio.svg)

## Dependencies

- GNU Readline
- libbsd
- SQLite3
- OpenSSL
- libuuid

#### Arch

```bash
sudo pacman -S lib32-sqlite lib32-openssl readline libbsd util-linux
```

#### Debian

```bash
sudo apt install libsqlite3-dev libssl-dev libreadline-dev libbsd-dev uuid-dev
```

## Usage

```bash
git clone https://github.com/masroof-maindak/jalebi.git
cd jalebi
make all
./jalebi      # server
./namak-paare # client
```

## TODOs

- [ ] Thread monitoring - Restart killed threads, etc.
    - How to find out a thread's died?
- [ ] close() -> shutdown() for sockets?
- [ ] Signal handlers; gracefully deal w/ sockets/readline(?)/database/threadpool/etc.

### Acknowledgements

- [uthash](https://github.com/troydhanson/uthash)

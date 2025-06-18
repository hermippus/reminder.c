# reminder.c
A simple reminder written in C.

It's just a pet project.

Dependencies:
- Some GNU/Linux distro
- Systemd
- Zenity

It also uses Unix sockets, signals, timerfd, and epoll.

```bash
# Install the reminder (requires root)
$ make install

# Start the reminder service (requires systemd)
$ systemctl --user start reminder

# Usage
$ reminder "Math" 13:00
```

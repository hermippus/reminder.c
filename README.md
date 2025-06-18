# reminder.c
Simple reminder written in C.

Dependencies:
- Some GNU/Linux distro
- Systemd
- Zenity

```bash
# Install the reminder (requires root)
$ make install

# Start the reminder service (requires systemd)
$ systemctl --user start reminder
```

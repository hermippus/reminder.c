CC		= clang
CFLAGS		= -std=c99 -Wall -Wextra 
INSTALL		= /usr/local/bin/
DAEMON		= /etc/systemd/system/

all: clean reminderd reminder

reminder:
	$(CC) $(CFLAGS) -o reminder cli/main.c

reminderd:
	$(CC) $(CFLAGS) -o reminderd daemon/main.c

install: all
	cp reminderd reminder $(INSTALL)
	cp ./service/reminder.service $(DAEMON)

clean:
	rm -f reminder reminderd

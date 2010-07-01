# Build libugci

OBJS		= ugci.o ugci-urefs.o
OBJSO		= ugci.lo ugci-urefs.lo
CC		= gcc
LD		= gcc
CFLAGS		= -Wall -O2 -D_GNU_SOURCE

PREFIX		= /usr

TARGET		= libugci.a
SOTARGET	= libugci.so
SOTARGETVER	= $(SOTARGET).0
PROGRAMS	= testugci setsecblk wdtimer dump_eeprom
INCLUDE		= ugci.h

ifdef DEBUG
CFLAGS += -DDEBUG -g
endif

.SUFFIXES: .c .o .lo

all: $(TARGET) $(PROGRAMS)

install: $(TARGET)
	install -D -m 644 $(TARGET) $(DESTDIR)$(PREFIX)/lib/$(TARGET)
	install -D -m 644 $(INCLUDE) $(DESTDIR)$(PREFIX)/include/$(INCLUDE)

installso: $(SOTARGET)
	install -D -m 644 $(SOTARGET) $(DESTDIR)$(PREFIX)/lib/$(SOTARGETVER)
	ln -fs $(SOTARGETVER) $(DESTDIR)$(PREFIX)/lib/$(SOTARGET)

$(TARGET): $(OBJS)
	$(AR) rc $@ $(OBJS)

.c.lo:
	$(CC) $(CFLAGS) -fPIC -DPIC -c $< -o $@

$(SOTARGET): $(OBJSO)
	$(LD) -Wl,-soname,$(SOTARGETVER) -shared $(OBJSO) -o $@

testugci: testugci.c $(TARGET)
	$(CC) $(CFLAGS) $+ -o $@

setsecblk: setsecblk.c $(TARGET)
	$(CC) $(CFLAGS) $+ -o $@

wdtimer: wdtimer.c $(TARGET)
	 $(CC) $(CFLAGS) $+ -o $@

dump_eeprom: dump_eeprom.c $(TARGET)
	$(CC) $(CFLAGS) $+ -o $@

clean:
	rm -f $(OBJS) $(OBJSO) $(TARGET) $(SOTARGET) $(PROGRAMS)

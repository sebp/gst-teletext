CC := gcc

GST_LIBS := $(shell pkg-config --libs gstreamer-0.10 gstreamer-interfaces-0.10)
GST_CFLAGS := $(shell pkg-config --cflags gstreamer-0.10 gstreamer-interfaces-0.10)
GTK_LIBS := $(shell pkg-config --libs gtk+-2.0)
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-2.0)
ZVBI_LIBS := $(shell pkg-config --libs zvbi-0.2)

CFLAGS := -g -Wall

test-ui: test-ui.o
test-ui: CFLAGS := $(CFLAGS) $(GTK_CFLAGS) $(GST_CFLAGS)
test-ui: LIBS := $(LIBS) $(GTK_LIBS) $(GST_LIBS) $(ZVBI_LIBS)

binaries += test-ui

all: $(binaries)

$(binaries):
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $^

%.o:: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
 
clean:
	rm -rf $(binaries)
	find . -name "*.o" | xargs rm -rf

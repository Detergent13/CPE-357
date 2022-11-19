SHELL = /bin/sh

SUBDIRS = src

LIBS = lib lib64

all:
	@if [ -n "$(SUBDIRS)" ]; then \
		for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir $@; done;\
	fi

clean:	localclean
	@if [ -n "$(SUBDIRS)" ]; then \
		for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir $@; done;\
	fi

allclean: localclean
	@if [ -n "$(SUBDIRS)" ]; then \
		for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir $@; done;\
	fi
	rm -rf $(LIBS)

localclean:
	rm -f *~

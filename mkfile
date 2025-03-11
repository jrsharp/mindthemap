</$objtype/mkfile

TARG=slides9
OFILES=\
	slides9.$O\

HFILES=\
	slides9.h\

BIN=/$objtype/bin
LDFLAGS=-ldraw -l9 -lmemdraw -lmemlayer -lkeyboard -levent

&lt;/sys/src/cmd/mkone


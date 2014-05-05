dnl $Id$
dnl config.m4 for extension uevent

PHP_ARG_ENABLE(uevent, whether to enable uevent support,
[  --enable-uevent           Enable uevent support])

if test "$PHP_UEVENT" != "no"; then
  PHP_NEW_EXTENSION(uevent, uevent.c, $ext_shared)
fi

#!/bin/sh
# Discard the --mode=xxx argument to libtool, and then run the original command
shift
exec $*

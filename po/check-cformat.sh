#!/bin/bash

# Check all translations for broken printf format strings

# Usually this is done by msgfmt, but it chokes on our %Tde and %P notations,
# so we disable it in the Makefile.

# This simple script temporarily replaces each %Tde with %s and each %P
# with %d, so that msgfmt can be run.

POFILES=$(grep '^POFILES =' Makefile|cut -d= -f2)

for po in ${POFILES}; do
  perl -p -e 's/%(([0-9]+$)?-?[0-9]*)[tT]../((t)%\1s/g; s/%(([0-9]+$)?-?[0-9]*)P/(P)%\1d/g' < ${po} > ${po}.tmp; msgfmt -c --statistics --verbose ${po}.tmp > /dev/null && rm -f ${po}.tmp
done
rm -f messages.mo

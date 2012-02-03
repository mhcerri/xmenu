#!/bin/bash

. /etc/bash_completion

if [[ "$#" -eq 1 ]]; then
    if OUT="$(compgen -c -- "$1")"; then
        echo "$OUT" | sort -u
        exit 0
    fi
    set -- $*
fi

PROG="$(basename "$1")"
FN="$(complete -p "$PROG" | grep -oE -- '-F *[^ ]+' | sed -e 's/-F //')"
COMP_WORDS=($*)
COMP_LINE="$*"
COMP_COUNT="$(echo "$COMP_LINE" | wc -c)"
COMP_CWORD=$(( $# -1 ))
if [[ "$COMP_CWORD" -lt 1 ]]; then
    COMP_CWORD=1
fi
$FN

echo "'$COMP_WORDS'" >&2
echo "'$COMP_LINE'" >&2
echo "'$COMP_COUNT'" >&2
echo "'$COMP_CWORD'" >&2

for ((i=0;i<${#COMPREPLY[*]};i++)); do
    echo ${COMPREPLY[i]}
done | sort -u

if [[ "${#COMPREPLY[*]}" -gt 0 ]]; then
    exit 0
fi

compgen -f -d -- ${!#} | sort -u


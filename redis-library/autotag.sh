#!/bin/bash
gcc -M "$1"  | sed -e 's/[\\ ]/\n/g' | \
	sed -e '/^$/d' -e '/\.o:[ \t]*$/d' | \
	ctags -a -L - 

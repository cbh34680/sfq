#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

php main.php

exit 0


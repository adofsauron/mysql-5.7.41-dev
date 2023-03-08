#!/bin/bash

pkill mysqld

sleep 1s

/usr/local/mysql/bin/mysqld --defaults-file=./my.cnf --user=mysql --datadir=/usr/local/share/mysql/data &

#!/bin/bash

rm /usr/local/share/mysql/data -rf

mkdir -p /usr/local/share/mysql/data 

chown -R mysql:mysql /usr/local/share/mysql/data 

sleep 1s

/usr/local/mysql/bin/mysqld --defaults-file=./my.cnf --initialize --user=mysql --datadir=/usr/local/share/mysql/data

chown -R mysql:mysql /usr/local/share/mysql/data 

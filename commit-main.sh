#!/bin/bash

git config --global core.autocrlf false

git config  --global core.fsmonitor true

git pull

git add .

git commit -m "fix mysql 5.7.41"

git push -u origin main




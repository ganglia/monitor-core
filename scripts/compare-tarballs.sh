#!/bin/bash


tar tzf $1 | cut -f2- -d/ > "${1}.file-list"
tar tzf $2 | cut -f2- -d/ > "${2}.file-list"

diff -ur "${1}.file-list" "${2}.file-list"


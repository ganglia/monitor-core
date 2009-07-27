#!/bin/bash

# This script renames all the host directory names to lowercase
# It is part of a solution to the case-sensitive hostname problem,
# bug 232, http://bugzilla.ganglia.info/cgi-bin/bugzilla/show_bug.cgi?id=232

# RRDROOT must be set to the location where RRD files are kept

#RRDROOT=/var/lib/ganglia/rrds

if [ -z "${RRDROOT}" ];
then
  echo Please specify RRDROOT
  exit 1
fi

killall gmetad
cd $RRDROOT || exit 1
find . -type d -name '*[A-Z]*' ! -name __SummaryInfo__ -mindepth 2 -maxdepth 2 | while read ;
do
  OLD_NAME=`echo "$REPLY" | cut -f3 -d/`
  NEW_NAME=`echo "$OLD_NAME" | tr [A-Z] [a-z]`
  CLUSTER_NAME=`echo "$REPLY" | cut -f2 -d/`
  mv "$REPLY" "${CLUSTER_NAME}/${NEW_NAME}"
done



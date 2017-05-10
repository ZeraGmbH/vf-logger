#!/bin/bash

SCRIPT_BASEDIR="$(dirname "$0")"
. "$SCRIPT_BASEDIR/docker.inc.sh"

if [ -d "`pwd`/psql" ]; then
    echo "Starting psql container"
    $DRY_RUN docker run --name $docker_container_name -v `pwd`/psql:/mnt/scripts:ro -d $docker_container_image
else
    echo "`pwd`/psql not found aborting"
    exit 1;
fi
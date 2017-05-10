#!/bin/bash

SCRIPT_BASEDIR="$(dirname "$0")"
. "$SCRIPT_BASEDIR/docker.inc.sh"

docker inspect -f {{.State.Running}} $docker_container_name > /dev/null || { exit 1; }
$DRY_RUN docker kill $docker_container_name
echo "Killed psql container"
$DRY_RUN docker rm $docker_container_name
echo "Removed psql container"
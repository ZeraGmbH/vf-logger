#!/bin/bash

SCRIPT_BASEDIR="$(dirname "$0")"
. "$SCRIPT_BASEDIR/docker.inc.sh"

echo "Running bash in container"
$DRY_RUN docker exec -it $docker_container_name bash
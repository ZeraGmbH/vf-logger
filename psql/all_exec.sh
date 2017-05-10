#!/bin/bash


SCRIPT_BASEDIR="$(dirname "$0")"

$SCRIPT_BASEDIR/create_user_db.sh testdb testuser testpass
cat $SCRIPT_BASEDIR/schema_test.psql | psql -Utestuser testdb
#cat $SCRIPT_BASEDIR/testdata.psql | psql -Utestuser testdb

echo "Done"

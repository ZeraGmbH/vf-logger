#========================================================
#Creating database :
#========================================================
#!/bin/bash

SCRIPT_BASEDIR="$(dirname "$0")"
. "$SCRIPT_BASEDIR/psql.inc.sh"

EXPECTED_ARGS=3

Q1="CREATE DATABASE $1;"
Q2="CREATE ROLE $2 ENCRYPTED PASSWORD '$3' NOSUPERUSER NOCREATEDB NOCREATEROLE INHERIT LOGIN;"
Q3="GRANT ALL PRIVILEGES ON DATABASE $1 TO $2;"

if [ $# -ne $EXPECTED_ARGS ]
then
echo -e "\nUsage: $0 dbname dbuser dbpass\n"
exit $E_BADARGS
fi

echo -e "\n${blue}-Creating mysql DATABASE ${red}${1}${norm}\n${blue}-Creating mysql USER ${red}${2}${norm}\n"
for query in "$Q1" "$Q2" "$Q3"
do
$SQL_BIN -U$SQL_USER -c "$query" && echo -e "$SQL_NAME -c $query ---> [${green}OK${norm}]" || echo -e "$query ---> [${red}BAD${norm}]"

done
echo -e " "

#========================================================
#List users:
#========================================================
#!/bin/bash

SCRIPT_BASEDIR="$(dirname "$0")"
. "$SCRIPT_BASEDIR/psql.inc.sh"

EXPECTED_ARGS=0

Q1="\du"

if [ $# -ne $EXPECTED_ARGS ]
then
echo -e "\nUsage: $0\n"
exit $E_BADARGS
fi

echo -e "\n${blue}-Listing users${norm}\n"
for query in "$Q1"
do
$SQL_BIN -U$SQL_USER -c "$query" && echo -e "$SQL_NAME -c $query ---> [${green}OK${norm}]" || echo -e "$query ---> [${red}BAD${norm}]"

done
echo -e " "

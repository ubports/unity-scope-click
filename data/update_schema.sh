#!/bin/sh
DBFILE=$1

if [ -z "$DBFILE" ]
then
    echo "Usage: $0 <departments db file>"
    exit 1
fi

if [ ! -w "$DBFILE" ]
then
    echo "Cannot open $DBFILE for writing"
    exit 2
fi

SCHEMA_VERSION=$(sqlite3 "$DBFILE" "SELECT value FROM meta WHERE name='version'")

##############################################
# Update departments db from version 1 to 2
##############################################
if [ "x$SCHEMA_VERSION" = "x1" ]
then
    sqlite3 "$DBFILE" << _UPDATE_TO_VER2
    BEGIN TRANSACTION;
    DROP VIEW depts_v;
    CREATE VIEW depts_v AS SELECT deptid, parentid FROM depts UNION SELECT depts2.parentid AS deptid,'' AS parentid FROM depts AS depts2 WHERE NOT
        EXISTS (SELECT * FROM depts WHERE depts.deptid=depts2.parentid);
    UPDATE meta SET value='2' WHERE name='version';
    END TRANSACTION;
_UPDATE_TO_VER2
fi

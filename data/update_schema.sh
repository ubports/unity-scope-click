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
    # updating deptnames may fail if en_US is already there, in such case just delete entries for empty locale
    sqlite3 "$DBFILE" "UPDATE deptnames SET locale='en_US' WHERE locale=''" || sqlite3 "$DBFILE" "DELETE FROM deptnames WHERE locale=''"
    sqlite3 "$DBFILE" << _UPDATE_TO_VER2
    BEGIN TRANSACTION;
    DROP VIEW depts_v;
    INSERT INTO depts SELECT deptid,'' FROM pkgmap WHERE NOT EXISTS (SELECT deptid FROM depts WHERE depts.deptid=pkgmap.deptid) GROUP BY deptid;
    UPDATE meta SET value='2' WHERE name='version';
    END TRANSACTION;
_UPDATE_TO_VER2
fi

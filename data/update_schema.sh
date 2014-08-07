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

    INSERT INTO deptnames (deptid,locale,name) VALUES ('universal-access','en_US','Universal Access');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('office','en_US','Office');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('graphics','en_US','Graphics');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('science-engineering','en_US','Science & Engineering');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('internet','en_US','Internet');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('accessories','en_US','Accessories');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('developer-tools','en_US','Developer Tools');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('games','en_US','Games');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('books-magazines','en_US','Books & Magazines');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('education','en_US','Education');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('sound-video','en_US','Sound & Video');

    BEGIN TRANSACTION;
    DROP VIEW depts_v;
    INSERT INTO depts (deptid,parentid) VALUES ('universal-access','');
    INSERT INTO depts (deptid,parentid) VALUES ('office','');
    INSERT INTO depts (deptid,parentid) VALUES ('graphics','');
    INSERT INTO depts (deptid,parentid) VALUES ('science-engineering','');
    INSERT INTO depts (deptid,parentid) VALUES ('internet','');
    INSERT INTO depts (deptid,parentid) VALUES ('accessories','');
    INSERT INTO depts (deptid,parentid) VALUES ('developer-tools','');
    INSERT INTO depts (deptid,parentid) VALUES ('games','');
    INSERT INTO depts (deptid,parentid) VALUES ('books-magazines','');
    INSERT INTO depts (deptid,parentid) VALUES ('education','');
    INSERT INTO depts (deptid,parentid) VALUES ('sound-video','');
    UPDATE meta SET value='2' WHERE name='version';
    END TRANSACTION;
_UPDATE_TO_VER2
fi

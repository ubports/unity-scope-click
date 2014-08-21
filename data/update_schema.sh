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
SCHEMA_VERSION=$(sqlite3 "$DBFILE" "SELECT value FROM meta WHERE name='version'")
fi

if [ "x$SCHEMA_VERSION" = "x2" ]
then
    sqlite3 "$DBFILE" << _UPDATE_TO_VER3
    BEGIN TRANSACTION;
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='3d';
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='computing-robotics';
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='debugging';
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='electronics';
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='engineering';
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='ides';
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='profiling';
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='version-control';
    UPDATE pkgmap SET deptid='developer-tools' WHERE deptid='web-development';
    UPDATE pkgmap SET deptid='science-engineering' WHERE deptid='astronomy';
    UPDATE pkgmap SET deptid='science-engineering' WHERE deptid='biology';
    UPDATE pkgmap SET deptid='science-engineering' WHERE deptid='chemistry';
    UPDATE pkgmap SET deptid='science-engineering' WHERE deptid='geography';
    UPDATE pkgmap SET deptid='science-engineering' WHERE deptid='geology';
    UPDATE pkgmap SET deptid='science-engineering' WHERE deptid='mathematics';
    UPDATE pkgmap SET deptid='science-engineering' WHERE deptid='medicine';
    UPDATE pkgmap SET deptid='science-engineering' WHERE deptid='physics';
    UPDATE pkgmap SET deptid='games' WHERE deptid='board-games';
    UPDATE pkgmap SET deptid='games' WHERE deptid='card-games';
    UPDATE pkgmap SET deptid='games' WHERE deptid='puzzles';
    UPDATE pkgmap SET deptid='games' WHERE deptid='role-playing';
    UPDATE pkgmap SET deptid='books-comics' WHERE deptid='books-magazines';
    UPDATE pkgmap SET deptid='communication' WHERE deptid='chat';
    UPDATE pkgmap SET deptid='communication' WHERE deptid='file-sharing';
    UPDATE pkgmap SET deptid='communication' WHERE deptid='internet';
    UPDATE pkgmap SET deptid='communication' WHERE deptid='mail';
    UPDATE pkgmap SET deptid='communication' WHERE deptid='web-browsers';
    UPDATE pkgmap SET deptid='graphics' WHERE deptid='drawing';
    UPDATE pkgmap SET deptid='graphics' WHERE deptid='graphic-interface-design';
    UPDATE pkgmap SET deptid='graphics' WHERE deptid='photography';
    UPDATE pkgmap SET deptid='graphics' WHERE deptid='publishing';
    UPDATE pkgmap SET deptid='graphics' WHERE deptid='scanning-ocr';
    UPDATE pkgmap SET deptid='personalisation' WHERE deptid='localization';
    UPDATE pkgmap SET deptid='personalisation' WHERE deptid='themes-tweaks';
    UPDATE pkgmap SET deptid='productivity' WHERE deptid='office';
    UPDATE pkgmap SET deptid='music-audio' WHERE deptid='sound-video';
    UPDATE pkgmap SET deptid='universal-accessaccessibility' WHERE deptid='universal-access';
    UPDATE pkgmap SET deptid='accessories' WHERE deptid='viewers';

    DELETE FROM depts;
    INSERT INTO depts (deptid,parentid) VALUES ('travel-local', '');
    INSERT INTO depts (deptid,parentid) VALUES ('productivity', '');
    INSERT INTO depts (deptid,parentid) VALUES ('finance', '');
    INSERT INTO depts (deptid,parentid) VALUES ('universal-accessaccessibility', '');
    INSERT INTO depts (deptid,parentid) VALUES ('entertainment', '');
    INSERT INTO depts (deptid,parentid) VALUES ('communication', '');
    INSERT INTO depts (deptid,parentid) VALUES ('graphics', '');
    INSERT INTO depts (deptid,parentid) VALUES ('science-engineering', '');
    INSERT INTO depts (deptid,parentid) VALUES ('news-magazines', '');
    INSERT INTO depts (deptid,parentid) VALUES ('accessories', '');
    INSERT INTO depts (deptid,parentid) VALUES ('sports', '');
    INSERT INTO depts (deptid,parentid) VALUES ('developer-tools', '');
    INSERT INTO depts (deptid,parentid) VALUES ('games', '');
    INSERT INTO depts (deptid,parentid) VALUES ('books-comics', '');
    INSERT INTO depts (deptid,parentid) VALUES ('music-audio', '');
    INSERT INTO depts (deptid,parentid) VALUES ('shopping', '');
    INSERT INTO depts (deptid,parentid) VALUES ('education', '');

    DELETE FROM deptnames;
    INSERT INTO deptnames (deptid,locale,name) VALUES ('books-comics','en_US','Books & Comics');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('business','en_US','Business');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('communication','en_US','Communication');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('developer-tools','en_US','Developer Tools');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('education','en_US','Education');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('entertainment','en_US','Entertainment');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('finance','en_US','Finance');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('food-drink','en_US','Food & Drink');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('games','en_US','Games');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('graphics','en_US','Graphics');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('health-fitness','en_US','Lifestyle');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('lifestyle','en_US','Health & Fitness');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('media-video','en_US','Media & Video');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('medical','en_US','Medical');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('music-audio','en_US','Music & Audio');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('news-magazines','en_US','News & Magazines');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('personalisation','en_US','Personalisation');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('productivity','en_US','Productivity');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('reference','en_US','Reference');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('science-engineering','en_US','Science & Engineering');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('shopping','en_US','Shopping');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('social-networking','en_US','Social Networking');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('sports','en_US','Sports');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('travel-local','en_US','Travel & Local');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('universal-accessaccessibility','en_US','Universal Access/Accessibility');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('weather','en_US','Weather');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('accessories','en_US','Utilities');

    UPDATE meta SET value='3' WHERE name='version';
    END TRANSACTION;
_UPDATE_TO_VER3
fi

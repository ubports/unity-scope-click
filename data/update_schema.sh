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
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Books & Comics','en_US','books-comics');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Business','en_US','business');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Communication','en_US','communication');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Developer Tools','en_US','developer-tools');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Education','en_US','education');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Entertainment','en_US','entertainment');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Finance','en_US','finance');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Food & Drink','en_US','food-drink');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Games','en_US','games');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Graphics','en_US','graphics');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Lifestyle','en_US','health-fitness');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Health & Fitness','en_US','lifestyle');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Media & Video','en_US','media-video');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Medical','en_US','medical');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Music & Audio','en_US','music-audio');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('News & Magazines','en_US','news-magazines');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Personalisation','en_US','personalisation');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Productivity','en_US','productivity');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Reference','en_US','reference');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Science & Engineering','en_US','science-engineering');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Shopping','en_US','shopping');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Social Networking','en_US','social-networking');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Sports','en_US','sports');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Travel & Local','en_US','travel-local');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Universal Access/Accessibility','en_US','universal-accessaccessibility');
    INSERT INTO deptnames (deptid,locale,name) VALUES ('Weather','en_US','weather');

    UPDATE meta SET value='3' WHERE name='version';
    END TRANSACTION;
_UPDATE_TO_VER3
fi

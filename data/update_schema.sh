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
SCHEMA_VERSION=$(sqlite3 "$DBFILE" "SELECT value FROM meta WHERE name='version'")
fi

if [ "x$SCHEMA_VERSION" = "x3" ]
then
    sqlite3 "$DBFILE" << _UPDATE_TO_VER4
    BEGIN TRANSACTION;

    DELETE FROM "deptnames";
    INSERT INTO "deptnames" VALUES('social-networking','en_US','Social Networking');
    INSERT INTO "deptnames" VALUES('travel-local','en_US','Travel & Local');
    INSERT INTO "deptnames" VALUES('reference','en_US','Reference');
    INSERT INTO "deptnames" VALUES('food-drink','en_US','Food & Drink');
    INSERT INTO "deptnames" VALUES('communication','en_US','Communication');
    INSERT INTO "deptnames" VALUES('accessories','en_US','Utilities');
    INSERT INTO "deptnames" VALUES('science-engineering','en_US','Science & Engineering');
    INSERT INTO "deptnames" VALUES('personalisation','en_US','Personalisation');
    INSERT INTO "deptnames" VALUES('education','en_US','Education');
    INSERT INTO "deptnames" VALUES('productivity','en_US','Productivity');
    INSERT INTO "deptnames" VALUES('universal-accessaccessibility','en_US','Universal Access/Accessibility');
    INSERT INTO "deptnames" VALUES('entertainment','en_US','Entertainment');
    INSERT INTO "deptnames" VALUES('sports','en_US','Sports');
    INSERT INTO "deptnames" VALUES('health-fitness','en_US','Health & Fitness');
    INSERT INTO "deptnames" VALUES('weather','en_US','Weather');
    INSERT INTO "deptnames" VALUES('shopping','en_US','Shopping');
    INSERT INTO "deptnames" VALUES('finance','en_US','Finance');
    INSERT INTO "deptnames" VALUES('business','en_US','Business');
    INSERT INTO "deptnames" VALUES('media-video','en_US','Media & Video');
    INSERT INTO "deptnames" VALUES('music-audio','en_US','Music & Audio');
    INSERT INTO "deptnames" VALUES('news-magazines','en_US','News & Magazines');
    INSERT INTO "deptnames" VALUES('graphics','en_US','Graphics');
    INSERT INTO "deptnames" VALUES('lifestyle','en_US','Lifestyle');
    INSERT INTO "deptnames" VALUES('medical','en_US','Medical');
    INSERT INTO "deptnames" VALUES('developer-tools','en_US','Developer Tools');
    INSERT INTO "deptnames" VALUES('games','en_US','Games');
    INSERT INTO "deptnames" VALUES('books-comics','en_US','Books & Comics');

    INSERT INTO "deptnames" VALUES('accessories','ca_ES','Utilitats');
    INSERT INTO "deptnames" VALUES('accessories','es_ES','Utilidades');
    INSERT INTO "deptnames" VALUES('accessories','eu_ES','Tresnak');
    INSERT INTO "deptnames" VALUES('accessories','gl_ES','Utilidades');
    INSERT INTO "deptnames" VALUES('books-comics','ca_ES','Llibres i còmics');
    INSERT INTO "deptnames" VALUES('books-comics','es_ES','Libros y cómics');
    INSERT INTO "deptnames" VALUES('books-comics','eu_ES','Liburuak eta komikiak');
    INSERT INTO "deptnames" VALUES('books-comics','gl_ES','Libros e cómics');
    INSERT INTO "deptnames" VALUES('communication','ca_ES','Comunicació');
    INSERT INTO "deptnames" VALUES('communication','es_ES','Comunicación');
    INSERT INTO "deptnames" VALUES('communication','eu_ES','Komunikazioa');
    INSERT INTO "deptnames" VALUES('communication','gl_ES','Comunicación');
    INSERT INTO "deptnames" VALUES('developer-tools','ca_ES','Eines per a desenvolupadors');
    INSERT INTO "deptnames" VALUES('developer-tools','es_ES','Herramientas para desarrolladores');
    INSERT INTO "deptnames" VALUES('developer-tools','eu_ES','Garatzaileentzako tresnak');
    INSERT INTO "deptnames" VALUES('developer-tools','gl_ES','Ferramentas de desenvolvemento');
    INSERT INTO "deptnames" VALUES('education','ca_ES','Educació');
    INSERT INTO "deptnames" VALUES('education','es_ES','Educación');
    INSERT INTO "deptnames" VALUES('education','eu_ES','Hezkuntza');
    INSERT INTO "deptnames" VALUES('education','gl_ES','Educación');
    INSERT INTO "deptnames" VALUES('entertainment','ca_ES','Entreteniment');
    INSERT INTO "deptnames" VALUES('entertainment','es_ES','Entretenimiento');
    INSERT INTO "deptnames" VALUES('entertainment','eu_ES','Entretenimendua');
    INSERT INTO "deptnames" VALUES('entertainment','gl_ES','Lecer');
    INSERT INTO "deptnames" VALUES('finance','ca_ES','Finances');
    INSERT INTO "deptnames" VALUES('finance','es_ES','Finanzas');
    INSERT INTO "deptnames" VALUES('finance','eu_ES','Finantzak');
    INSERT INTO "deptnames" VALUES('finance','gl_ES','Finanzas');
    INSERT INTO "deptnames" VALUES('food-drink','ca_ES','Menjar i begudes');
    INSERT INTO "deptnames" VALUES('food-drink','es_ES','Comida y bebida');
    INSERT INTO "deptnames" VALUES('food-drink','eu_ES','Jana eta edaria');
    INSERT INTO "deptnames" VALUES('food-drink','gl_ES','Comida & bebida');
    INSERT INTO "deptnames" VALUES('games','ca_ES','Jocs');
    INSERT INTO "deptnames" VALUES('games','es_ES','Juegos');
    INSERT INTO "deptnames" VALUES('games','eu_ES','Jokoak');
    INSERT INTO "deptnames" VALUES('games','gl_ES','Xogos');
    INSERT INTO "deptnames" VALUES('graphics','ca_ES','Gràfics');
    INSERT INTO "deptnames" VALUES('graphics','es_ES','Gráficos');
    INSERT INTO "deptnames" VALUES('graphics','eu_ES','Grafikoak');
    INSERT INTO "deptnames" VALUES('graphics','gl_ES','Gráficos');
    INSERT INTO "deptnames" VALUES('lifestyle','ca_ES','Estil de vida');
    INSERT INTO "deptnames" VALUES('lifestyle','es_ES','Estilo de vida');
    INSERT INTO "deptnames" VALUES('lifestyle','eu_ES','Bizi-estiloa');
    INSERT INTO "deptnames" VALUES('lifestyle','gl_ES','Estilo de vida');
    INSERT INTO "deptnames" VALUES('media-video','ca_ES','Multimèdia i vídeo');
    INSERT INTO "deptnames" VALUES('media-video','es_ES','Multimedia y vídeos');
    INSERT INTO "deptnames" VALUES('media-video','eu_ES','Multimedia eta bideoak');
    INSERT INTO "deptnames" VALUES('media-video','gl_ES','Multimedia e vídeo');
    INSERT INTO "deptnames" VALUES('medical','ca_ES','Medicina');
    INSERT INTO "deptnames" VALUES('medical','es_ES','Medicina');
    INSERT INTO "deptnames" VALUES('medical','eu_ES','Osasuna');
    INSERT INTO "deptnames" VALUES('medical','gl_ES','Medicina');
    INSERT INTO "deptnames" VALUES('music-audio','ca_ES','Música i àudio');
    INSERT INTO "deptnames" VALUES('music-audio','es_ES','Música y audio');
    INSERT INTO "deptnames" VALUES('music-audio','eu_ES','Musika eta audioa');
    INSERT INTO "deptnames" VALUES('music-audio','gl_ES','Música e son');
    INSERT INTO "deptnames" VALUES('news-magazines','ca_ES','Notícies i revistes');
    INSERT INTO "deptnames" VALUES('news-magazines','es_ES','Noticias y revistas');
    INSERT INTO "deptnames" VALUES('news-magazines','eu_ES','Albisteak eta aldizkariak');
    INSERT INTO "deptnames" VALUES('news-magazines','gl_ES','Novas e revistas');
    INSERT INTO "deptnames" VALUES('productivity','ca_ES','Productivitat');
    INSERT INTO "deptnames" VALUES('productivity','es_ES','Productividad');
    INSERT INTO "deptnames" VALUES('productivity','eu_ES','Produktibitatea');
    INSERT INTO "deptnames" VALUES('productivity','gl_ES','Produtividade');
    INSERT INTO "deptnames" VALUES('reference','ca_ES','Referència');
    INSERT INTO "deptnames" VALUES('reference','es_ES','Referencia');
    INSERT INTO "deptnames" VALUES('reference','eu_ES','Erreferentzia');
    INSERT INTO "deptnames" VALUES('reference','gl_ES','Referencia');
    INSERT INTO "deptnames" VALUES('science-engineering','ca_ES','Ciència i enginyeria');
    INSERT INTO "deptnames" VALUES('science-engineering','es_ES','Ciencia e ingeniería');
    INSERT INTO "deptnames" VALUES('science-engineering','eu_ES','Zientzia eta ingeniaritza');
    INSERT INTO "deptnames" VALUES('science-engineering','gl_ES','Ciencia e enxeñaría');
    INSERT INTO "deptnames" VALUES('shopping','ca_ES','Compres');
    INSERT INTO "deptnames" VALUES('shopping','es_ES','Tiendas');
    INSERT INTO "deptnames" VALUES('shopping','eu_ES','Erosketak');
    INSERT INTO "deptnames" VALUES('shopping','gl_ES','Compras');
    INSERT INTO "deptnames" VALUES('social-networking','ca_ES','Xarxes socials');
    INSERT INTO "deptnames" VALUES('social-networking','es_ES','Redes sociales');
    INSERT INTO "deptnames" VALUES('social-networking','eu_ES','Sare sozialak');
    INSERT INTO "deptnames" VALUES('social-networking','gl_ES','Redes sociais');
    INSERT INTO "deptnames" VALUES('sports','ca_ES','Esports');
    INSERT INTO "deptnames" VALUES('sports','es_ES','Deportes');
    INSERT INTO "deptnames" VALUES('sports','eu_ES','Kirolak');
    INSERT INTO "deptnames" VALUES('sports','gl_ES','Deportes');
    INSERT INTO "deptnames" VALUES('travel-local','ca_ES','Viatges i regional');
    INSERT INTO "deptnames" VALUES('travel-local','es_ES','Viajes y guías');
    INSERT INTO "deptnames" VALUES('travel-local','eu_ES','Bidaiak eta gidak');
    INSERT INTO "deptnames" VALUES('travel-local','gl_ES','Viaxes e local');
    INSERT INTO "deptnames" VALUES('universal-accessaccessibility','ca_ES','Accés universal i accessibilitat');
    INSERT INTO "deptnames" VALUES('universal-accessaccessibility','es_ES','Acceso universal/accesibilidad');
    INSERT INTO "deptnames" VALUES('universal-accessaccessibility','eu_ES','Sarbide unibertsala/Erabilerraztasuna');
    INSERT INTO "deptnames" VALUES('universal-accessaccessibility','gl_ES','Acceso universal/Accesibilidade');
    INSERT INTO "deptnames" VALUES('weather','ca_ES','El temps');
    INSERT INTO "deptnames" VALUES('weather','es_ES','Meteorología');
    INSERT INTO "deptnames" VALUES('weather','eu_ES','Eguraldia');
    INSERT INTO "deptnames" VALUES('weather','gl_ES','O Tempo');

    DELETE FROM "pkgmap";
    INSERT INTO "pkgmap" VALUES('com.canonical.cincodias','news-magazines');
    INSERT INTO "pkgmap" VALUES('com.canonical.elpais','news-magazines');
    INSERT INTO "pkgmap" VALUES('com.nokia.heremaps','travel-local');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.webapps.webapp-amazon','shopping');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.webapps.webapp-amazon-es','shopping');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.webapps.webapp-amazon-int','shopping');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.dropping-letters','games');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.filemanager','accesories');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.shorts','news-magazines');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.sudoku','games');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.telegram','communication');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.terminal','accesories');
    INSERT INTO "pkgmap" VALUES('com.zeptolab.cuttherope.free','games');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.weather','weather');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.clock','accessories');
    INSERT INTO "pkgmap" VALUES('com.canonical.payui','accessories');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.music','music-audio');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.webapps.webapp-ebay','shopping');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.webapps.webapp-gmail','communication');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.ken-vandine.pathwind','games');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.gallery','accessories');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.camera','accessories');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.calculator','accessories');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.mzanetti.tagger','accessories');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.webapps.webapp-facebook','communication');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.reminders','accessories');
    INSERT INTO "pkgmap" VALUES('com.ubuntu.developer.webapps.webapp-twitter','communication');
    INSERT INTO "pkgmap" VALUES('address-book-app.desktop','accessories');
    INSERT INTO "pkgmap" VALUES('dialer-app.desktop','communication');
    INSERT INTO "pkgmap" VALUES('mediaplayer-app.desktop','sound-video');
    INSERT INTO "pkgmap" VALUES('messaging-app.desktop','communication');
    INSERT INTO "pkgmap" VALUES('ubuntu-system-settings.desktop','accessories');
    INSERT INTO "pkgmap" VALUES('webbrowser-app.desktop','communication');

    UPDATE meta SET value='4' WHERE name='version';
    END TRANSACTION;
_UPDATE_TO_VER4
SCHEMA_VERSION=$(sqlite3 "$DBFILE" "SELECT value FROM meta WHERE name='version'")
fi

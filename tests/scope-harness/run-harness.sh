#!/bin/sh
#
# Copyright (C) 2015 Canonical Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
set -x

SOURCE_DIR=$1

if [ -z "${SOURCE_DIR}" ]; then
    echo "No source directory specified."
    exit 1
fi

CWD=`pwd`
TEMP_DIR=`mktemp -d --tmpdir=${CWD} test.XXXXXX`

# Copy applications test data over
mkdir -p ${TEMP_DIR}/applications
cp -a ${SOURCE_DIR}/applications/*.desktop ${TEMP_DIR}/applications

# Make the cache directory
mkdir -p ${TEMP_DIR}/cache

# Copy departments db
cp -a ${SOURCE_DIR}/../../data/departments.db ${TEMP_DIR}/cache/click-departments.db

export XDG_DATA_HOME=${TEMP_DIR}
export XDG_CACHE_HOME=${TEMP_DIR}/cache
export XDG_CONFIG_HOME=${TEMP_DIR}/config

(cd ${SOURCE_DIR} && xvfb-run -a python3 -m testtools.run discover)

RESULT=$?

rm -rf ${TEMP_DIR}

exit $RESULT

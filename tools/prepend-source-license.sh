#!/usr/bin/env bash -e

if [ -z "${1}" ]; then
    echo "No file argument provided"
    exit 1
fi

file=$1

tmp_file=${file}.tmp

set -u

mv ${file} ${tmp_file}

function put()
{
    echo ${1} >> ${file}
}

put "// ============================================================================="
put "// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>"
put "//"
put "// SPDX-License-Identifier: AGPL-3.0-or-later"
put "// ============================================================================="
put ""

cat ${tmp_file} >> ${file}

rm ${tmp_file}

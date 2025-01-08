#!/usr/bin/env bash

set -ex

touch icons.h

cat << EOF > icons.h
#pragma once

#include <map>
#include <string>

static const std::map<std::string, std::string> icons = {
EOF

curl "https://www.nerdfonts.com/cheat-sheet" | grep -oP '.*nf-linux-\K.+' | while read -r i; do
  name="$(echo "$i" | grep -oP '^[^"]+' | tr -d ' ' | tr -d '-' | tr -d '_' )"
  codepoint="$(echo $i | grep -oP '[^"]+"[^"]*$' | grep -oP '^[^"]+')"
  echo "    { \"$name\", \"\u$codepoint\" }," >> icons.h
done

cat << EOF >> icons.h
};
EOF

# If it exists, use clang-format to prettify icons.h
if hash clang-format 2>/dev/null; then
  tempfile=$(mktemp --suffix=.h)
  clang-format icons.h -style="gnu" > "$tempfile"
  mv "$tempfile" icons.h
fi

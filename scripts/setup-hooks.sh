#!/bin/sh
root="`hg root`"
dir="$root/.hg"

echo '[hooks'] >> "$dir/hgrc"
echo "precommit.clang-format = "$root"/scripts/clang-format-hook" >> "$dir/hgrc"
echo "hook scripts set up in $dir/hgrc" 

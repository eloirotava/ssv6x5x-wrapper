#!/bin/sh
# gera patches/NOME com o que mudou em work/ desde o ultimo patch (stage/)
set -e
cd "$(dirname "$0")"
diff -ruN stage work | sed -E 's#^(---|\+\+\+) (stage|work)/([^\t]*)\t.*#\1 \2/\3#' > "patches/$1" || true
rm -rf stage && cp -a work stage
grep -c '^[-+][^-+]' "patches/$1"

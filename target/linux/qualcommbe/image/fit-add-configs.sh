#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
#
# Duplicate one configuration node of a mkits.sh-generated .its file under
# more names, so the FIT boots whichever configuration name the vendor
# U-Boot asks for.
#
# usage: fit-add-configs.sh <file.its> <existing-config> <new-config>...

set -e

its="$1"
src="$2"
shift 2

[ -f "$its" ] || { echo "$0: $its not found" >&2; exit 1; }
grep -q "^[[:space:]]*${src} {" "$its" || {
	echo "$0: configuration '$src' not found in $its" >&2
	exit 1
}

awk -v src="$src" -v names="$*" '
BEGIN { n = split(names, extra, " ") }
{
	print
	line = $0
	sub(/^[[:space:]]+/, "", line)
	if (!grabbing && line == src " {") {
		grabbing = 1
		indent = $0
		sub(/[^[:space:]].*$/, "", indent)
		body = ""
		next
	}
	if (grabbing) {
		if ($0 == indent "};") {
			grabbing = 0
			for (i = 1; i <= n; i++)
				printf "%s%s {\n%s%s};\n", indent, extra[i], body, indent
			next
		}
		body = body $0 "\n"
	}
}
' "$its" > "$its.new"

mv "$its.new" "$its"

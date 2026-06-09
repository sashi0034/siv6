#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$script_dir"

ignore_patterns=()
if [[ -f .clang-format-ignore ]]; then
	while IFS= read -r pattern || [[ -n "$pattern" ]]; do
		pattern="${pattern%$'\r'}"

		case "$pattern" in
			"" | \#*)
				continue
				;;
		esac

		ignore_patterns+=("$pattern")
	done < .clang-format-ignore
fi

is_ignored() {
	local file="$1"
	local pattern

	for pattern in "${ignore_patterns[@]}"; do
		if [[ "$file" == $pattern ]]; then
			return 0
		fi
	done

	return 1
}

files=()
while IFS= read -r -d '' file; do
	if is_ignored "$file"; then
		continue
	fi

	files+=("$file")
done < <(
	git ls-files -z \
		'*.c' '*.cc' '*.cpp' '*.cxx' \
		'*.h' '*.hh' '*.hpp' '*.hxx' \
		'*.ipp' '*.inl'
)

batch=()
for file in "${files[@]}"; do
	batch+=("$file")

	if ((${#batch[@]} >= 100)); then
		clang-format -i "${batch[@]}"
		batch=()
	fi
done

if ((${#batch[@]})); then
	clang-format -i "${batch[@]}"
fi

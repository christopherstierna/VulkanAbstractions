
#!/usr/bin/env bash

SHADER_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$SHADER_DIR/../Spv"
FAILED=0

mkdir -p "$OUTPUT_DIR"

while IFS= read -r -d '' FILE; do
    # Skip files inside Include directories
    if [[ "$FILE" == */Include/* ]]; then
        echo "Skipping $FILE"
        continue
    fi

    FILENAME="$(basename "$FILE")"
    OUTPUT="$OUTPUT_DIR/${FILENAME%.slang}.spv"

    echo "Compiling $FILE..."

    if slangc \
        "$FILE" \
        -target spirv \
        -profile spirv_1_6 \
        -fvk-use-entrypoint-name \
        -O3 \
        -o "$OUTPUT"; then
        echo "    OK: $OUTPUT"
    else
        echo "    FAILED: $FILENAME"
        FAILED=1
    fi

done < <(find "$SHADER_DIR" -type f -name '*.slang' -print0)

exit "$FAILED"
#!/usr/bin/env bash
# Snail Pie DS - Real-time GameMaker / Deltarune asset pipeline and compiler for Nintendo DS
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python3 "$SCRIPT_DIR/snail_pie_ds/snail_pie_ds.py" "$@"

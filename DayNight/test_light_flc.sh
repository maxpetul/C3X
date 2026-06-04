#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./test_light_flx.sh --in <annotation_lights.pcx> --out-dir <out_dir> [options]

Required:
  --in PATH                 Input *_lights.pcx annotation sheet
  --out-dir PATH            Output directory for generated .flc files

Options:
  --rows N                  Number of rows in sheet (default: 1)
  --cols N                  Number of columns in sheet (default: 1)
  --cell-w N                Cell width (default: 128)
  --cell-h N                Cell height (default: 64)
  --frames N                Animation frames (default: 12)
  --fps N                   FPS metadata value (default: 12)
  --frame-change-rate N     0..1 temporal smoothing (default: 1.0 for clear motion)
  --hz1 N                   Primary flicker frequency in Hz (default: 1.2)
  --hz2 N                   Secondary flicker frequency in Hz (default: 2.4)
  --name-prefix NAME        Output name prefix (default: Lights)
  --with-ring-frame         Append ring frame
  --python CMD              Python executable (default: python3)
  --help                    Show this help

Any unrecognized args are passed through to pcx_sheet_to_civ3_flc_flicker.py.
EOF
}

INP=""
OUT_DIR=""
ROWS=1
COLS=1
CELL_W=128
CELL_H=64
FRAMES=12
FPS=12
FRAME_CHANGE_RATE=1.0
HZ1=1.2
HZ2=2.4
NAME_PREFIX="Lights"
WITH_RING_FRAME=0
PYTHON_BIN="python"

EXTRA_ARGS=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --in) INP="${2:-}"; shift 2 ;;
    --out-dir) OUT_DIR="${2:-}"; shift 2 ;;
    --rows) ROWS="${2:-}"; shift 2 ;;
    --cols) COLS="${2:-}"; shift 2 ;;
    --cell-w) CELL_W="${2:-}"; shift 2 ;;
    --cell-h) CELL_H="${2:-}"; shift 2 ;;
    --frames) FRAMES="${2:-}"; shift 2 ;;
    --fps) FPS="${2:-}"; shift 2 ;;
    --frame-change-rate) FRAME_CHANGE_RATE="${2:-}"; shift 2 ;;
    --hz1) HZ1="${2:-}"; shift 2 ;;
    --hz2) HZ2="${2:-}"; shift 2 ;;
    --name-prefix) NAME_PREFIX="${2:-}"; shift 2 ;;
    --with-ring-frame) WITH_RING_FRAME=1; shift ;;
    --python) PYTHON_BIN="${2:-}"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) EXTRA_ARGS+=("$1"); shift ;;
  esac
done

if [[ -z "$INP" || -z "$OUT_DIR" ]]; then
  usage
  exit 1
fi

if [[ ! -f "$INP" ]]; then
  echo "Input file not found: $INP" >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FLICKER_SCRIPT="$SCRIPT_DIR/pcx_sheet_to_civ3_flc_flicker.py"

if [[ ! -f "$FLICKER_SCRIPT" ]]; then
  echo "Missing script: $FLICKER_SCRIPT" >&2
  exit 1
fi

# Match generate_light_pcx.sh key configuration.
LIGHT_KEYS=(
  "#F6915E" # Orange
  "#FEF500" # Yellow
  "#00feff" # Teal
  "#E4080A" # Red
  "#BD15D0" # Purple
  "#2D9C01" # Green
  "#FF25C8" # Pink
  "#0A02EB" # Blue
  "#8262ED" # Indigo
)

LIGHT_STYLES=(
  "key=#F6915E; core=#ff8a20; glow=#dc6a00; core_gain=1.0; highlight_gain=0.0; size_radius=1.5; size_boost=0.05; halo_gain=6.0; halo_radius=1.0; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"
  "key=#FEF500; core=#ff8a20; glow=#dc6a00; core_gain=2.5; highlight_gain=1.0; size_radius=6.5; size_boost=1.5; halo_gain=20.0; halo_radius=0.1; core_radius=1.1; halo_gamma=1.3; size_gamma=0.75;"
  "key=#E4080A; core=#E4080A; glow=#E4080A; core_gain=1.0; highlight_gain=0.0; size_radius=0.5; size_boost=0.0; halo_gain=6.0; halo_radius=1.0; core_radius=0.5; halo_gamma=0.9; size_gamma=0.0; blend_mode=add;"
  "key=#00feff; core=#00feff; glow=#00feff; core_gain=0.6; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"
  "key=#BD15D0; core=#BD15D0; glow=#BD15D0; core_gain=0.6; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"
  "key=#2D9C01; core=#2D9C01; glow=#2D9C01; core_gain=0.6; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"
  "key=#FF25C8; core=#FF25C8; glow=#FF25C8; core_gain=0.6; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"
  "key=#0A02EB; core=#0A02EB; glow=#0A02EB; core_gain=0.2; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"
  "key=#8262ED; core=#7521DC; glow=#7521DC; core_gain=0.2; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"
)

LK_ARGS=()
for c in "${LIGHT_KEYS[@]}"; do
  LK_ARGS+=(--light-key "$c")
done

STYLE_ARGS=()
for s in "${LIGHT_STYLES[@]}"; do
  STYLE_ARGS+=(--light-style "$s")
done

# Match generate_light_pcx.sh global city-light defaults.
GLOBAL_ARGS=(
  --core-color "#ff8a20"
  --glow-color "#dc6a00"
  --core-radius 1.1
  --halo-radius 13.0
  --core-gain 2.5
  --halo-gain 20.0
  --highlight-gain 0.5
  --size-boost 1.7
  --size-radius 6.5
  --size-gamma 0.75
  --halo-sep 0.75
  --halo-gamma 1.3
  --blend-mode screen
)

CMD=(
  "$PYTHON_BIN" "$FLICKER_SCRIPT"
  --in "$INP"
  --out-dir "$OUT_DIR"
  --rows "$ROWS"
  --cols "$COLS"
  --cell-w "$CELL_W"
  --cell-h "$CELL_H"
  --frames "$FRAMES"
  --fps "$FPS"
  --frame-change-rate "$FRAME_CHANGE_RATE"
  --hz1 "$HZ1"
  --hz2 "$HZ2"
  --name-prefix "$NAME_PREFIX"
  "${GLOBAL_ARGS[@]}"
  "${LK_ARGS[@]}"
  "${STYLE_ARGS[@]}"
  "${EXTRA_ARGS[@]}"
)

if [[ "$WITH_RING_FRAME" -eq 1 ]]; then
  CMD+=(--with-ring-frame)
fi

echo "Running:"
printf ' %q' "${CMD[@]}"
echo
"${CMD[@]}"

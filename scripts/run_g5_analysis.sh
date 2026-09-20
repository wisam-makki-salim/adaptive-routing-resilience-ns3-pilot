#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ns3_root="${NS3_ROOT:-${project_root}/../tools/ns-allinone-3.47/ns-3.47}"
trace_dir="${project_root}/results/g5/traces"
mkdir -p "${trace_dir}" "${project_root}/figures"
export PATH="${HOME}/.local/bin:${PATH}"

cp "${project_root}/src/lra_olsr_controller.h" "${ns3_root}/scratch/lra_olsr_controller.h"
cp "${project_root}/src/g4_controlled_experiment.cc" "${ns3_root}/scratch/g4_controlled_experiment.cc"
(
  cd "${ns3_root}"
  ./ns3 build g4_controlled_experiment
  for mechanism in baseline adaptive; do
    ./ns3 run --no-build \
      "g4_controlled_experiment --scenario=C --mechanism=${mechanism} --run=102 --tracePrefix=${trace_dir}/C_${mechanism}_102" \
      2>/dev/null | tail -n 1 > "${trace_dir}/C_${mechanism}_102.row"
  done
)

python3 "${project_root}/scripts/analyze_g5.py" \
  "${project_root}/results/g4/raw/g4_final_runs.csv" \
  "${project_root}/results/g5" \
  "${project_root}/figures" \
  "${trace_dir}"

stored="$(awk -F, '$1=="C" && $3==102 {print}' "${project_root}/results/g4/raw/g4_final_runs.csv" | sort)"
rerun="$(cat "${trace_dir}"/*.row | sort)"
test "${stored}" = "${rerun}"
printf '%s\n' 'G5_RERUN_MATCH=PASS' 'G5_ANALYSIS=PASS' > "${project_root}/results/g5/g5_validation.txt"

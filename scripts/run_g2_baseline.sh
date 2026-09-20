#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ns3_root="${NS3_ROOT:-${project_root}/../tools/ns-allinone-3.47/ns-3.47}"
raw_dir="${project_root}/results/g2/raw"
mkdir -p "${raw_dir}"
export PATH="${HOME}/.local/bin:${PATH}"

cp "${project_root}/src/g2_baseline_scenario_a.cc" \
  "${ns3_root}/scratch/g2_baseline_scenario_a.cc"

(
  cd "${ns3_root}"
  ./ns3 build g2_baseline_scenario_a
)

header="run,tx_packets,rx_packets,lost_packets,pdr_percent,min_flow_pdr_percent,mean_delay_ms,delay_p50_ms,delay_p95_ms,goodput_kbps,olsr_control_packets,olsr_control_bytes,normalized_overhead_bytes_per_delivered_payload_byte,next_hop_changes,route_unavailable_samples"
printf '%s\n' "${header}" > "${raw_dir}/scenario_a_development_runs.csv"
for run in 1 2 3 4 5; do
  result="$(cd "${ns3_root}" && ./ns3 run "g2_baseline_scenario_a --run=${run}" 2>/dev/null | tail -n 1)"
  printf '%s\n' "${result}" >> "${raw_dir}/scenario_a_development_runs.csv"
done

python3 "${project_root}/scripts/summarize_g2.py" \
  "${raw_dir}/scenario_a_development_runs.csv" \
  "${project_root}/results/g2/baseline_summary.csv"

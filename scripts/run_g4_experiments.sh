#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ns3_root="${NS3_ROOT:-${project_root}/../tools/ns-allinone-3.47/ns-3.47}"
raw_dir="${project_root}/results/g4/raw"
mkdir -p "${raw_dir}"
export PATH="${HOME}/.local/bin:${PATH}"
parallel_jobs="${G4_JOBS:-6}"

cp "${project_root}/src/lra_olsr_controller.h" "${ns3_root}/scratch/lra_olsr_controller.h"
cp "${project_root}/src/g4_controlled_experiment.cc" "${ns3_root}/scratch/g4_controlled_experiment.cc"
(
  cd "${ns3_root}"
  ./ns3 build g4_controlled_experiment
)

header="scenario,mechanism,run,tx_packets,rx_packets,lost_packets,pdr_percent,min_flow_pdr_percent,mean_delay_ms,delay_p50_ms,delay_p95_ms,goodput_kbps,olsr_control_packets,olsr_control_bytes,normalized_overhead,recovery_seconds,next_hop_changes,route_unavailable_samples,signal_transitions,failure_transitions,recoveries,disrupted_node"
printf '%s\n' "${header}" > "${raw_dir}/g4_final_runs.csv"
task_file="${raw_dir}/g4_tasks.txt"
: > "${task_file}"
for scenario in A B C; do
  for mechanism in baseline adaptive; do
    for run in $(seq 101 120); do
      printf '%s %s %s\n' "${scenario}" "${mechanism}" "${run}" >> "${task_file}"
    done
  done
done

export ns3_root raw_dir
xargs -P "${parallel_jobs}" -n 3 bash -c '
  scenario="$1"; mechanism="$2"; run="$3"
  cd "${ns3_root}"
  ./ns3 run --no-build "g4_controlled_experiment --scenario=${scenario} --mechanism=${mechanism} --run=${run}" 2>/dev/null \
    | tail -n 1 > "${raw_dir}/${scenario}_${mechanism}_${run}.row"
' _ < "${task_file}"

for scenario in A B C; do
  for mechanism in baseline adaptive; do
    for run in $(seq 101 120); do
      cat "${raw_dir}/${scenario}_${mechanism}_${run}.row" >> "${raw_dir}/g4_final_runs.csv"
    done
  done
done

python3 "${project_root}/scripts/analyze_g4.py" \
  "${raw_dir}/g4_final_runs.csv" \
  "${project_root}/results/g4/g4_summary.csv" \
  "${project_root}/results/g4/g4_paired_differences.csv"

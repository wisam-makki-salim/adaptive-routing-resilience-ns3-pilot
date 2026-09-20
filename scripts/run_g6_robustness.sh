#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ns3_root="${NS3_ROOT:-${project_root}/../tools/ns-allinone-3.47/ns-3.47}"
raw_dir="${project_root}/results/g6/raw"
mkdir -p "${raw_dir}" "${project_root}/figures"
export PATH="${HOME}/.local/bin:${PATH}"
parallel_jobs="${G6_JOBS:-6}"

cp "${project_root}/src/lra_olsr_controller.h" "${ns3_root}/scratch/lra_olsr_controller.h"
cp "${project_root}/src/g4_controlled_experiment.cc" "${ns3_root}/scratch/g4_controlled_experiment.cc"
(
  cd "${ns3_root}"
  ./ns3 build g4_controlled_experiment
)

header="scenario,variant,run,tx_packets,rx_packets,lost_packets,pdr_percent,min_flow_pdr_percent,mean_delay_ms,delay_p50_ms,delay_p95_ms,goodput_kbps,olsr_control_packets,olsr_control_bytes,normalized_overhead,recovery_seconds,next_hop_changes,route_unavailable_samples,signal_transitions,failure_transitions,recoveries,disrupted_node"
printf '%s\n' "${header}" > "${raw_dir}/g6_runs.csv"
task_file="${raw_dir}/g6_tasks.tsv"
: > "${task_file}"

for run in $(seq 101 120); do
  printf 'C\tdetection_only\t%s\t--reactiveHelloSeconds=2.0 --reactiveTcSeconds=5.0\n' "${run}" >> "${task_file}"
  printf 'C\thello_only\t%s\t--reactiveHelloSeconds=0.5 --reactiveTcSeconds=5.0\n' "${run}" >> "${task_file}"
  printf 'C\ttc_only\t%s\t--reactiveHelloSeconds=2.0 --reactiveTcSeconds=1.0\n' "${run}" >> "${task_file}"
  printf 'C\tfailure20\t%s\t--finalFailureThreshold=20\n' "${run}" >> "${task_file}"
  printf 'C\tfailure80\t%s\t--finalFailureThreshold=80\n' "${run}" >> "${task_file}"
  printf 'B\tsignal78\t%s\t--riskThresholdDbm=-78.0\n' "${run}" >> "${task_file}"
  printf 'B\tsignal84\t%s\t--riskThresholdDbm=-84.0\n' "${run}" >> "${task_file}"
done
printf 'C\tfailure_disabled\t102\t--enableFailureTrigger=false\n' >> "${task_file}"

export ns3_root raw_dir
xargs -P "${parallel_jobs}" -d '\n' -I '{}' bash -c '
  IFS=$'"'"'\t'"'"' read -r scenario variant run extra <<< "$1"
  cd "${ns3_root}"
  ./ns3 run --no-build "g4_controlled_experiment --scenario=${scenario} --mechanism=adaptive --run=${run} ${extra}" 2>/dev/null \
    | tail -n 1 | sed "s/^${scenario},adaptive,${run},/${scenario},${variant},${run},/" \
    > "${raw_dir}/${scenario}_${variant}_${run}.row"
' _ '{}' < "${task_file}"

while IFS=$'\t' read -r scenario variant run extra; do
  cat "${raw_dir}/${scenario}_${variant}_${run}.row" >> "${raw_dir}/g6_runs.csv"
done < "${task_file}"

python3 "${project_root}/scripts/analyze_g6.py" \
  "${project_root}/results/g4/raw/g4_final_runs.csv" \
  "${raw_dir}/g6_runs.csv" \
  "${project_root}/results/g6" \
  "${project_root}/figures"

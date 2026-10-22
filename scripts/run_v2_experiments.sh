#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ns3_root="${NS3_ROOT:-${project_root}/../tools/ns-allinone-3.47/ns-3.47}"
raw_dir="${project_root}/results/v2/raw"
log_dir="${project_root}/results/v2/logs"
parallel_jobs="${V2_JOBS:-6}"
mkdir -p "${raw_dir}" "${log_dir}" "${project_root}/figures/v2"

test -x "${ns3_root}/ns3" || {
  printf 'ns-3 launcher not found at %s\n' "${ns3_root}" >&2
  exit 2
}

cp "${project_root}/src/lra_olsr_controller.h" "${ns3_root}/scratch/lra_olsr_controller.h"
cp "${project_root}/src/g4_controlled_experiment.cc" "${ns3_root}/scratch/g4_controlled_experiment.cc"
(
  cd "${ns3_root}"
  ./ns3 build g4_controlled_experiment
)

header="scenario,mechanism,run,tx_packets,rx_packets,lost_packets,pdr_percent,min_flow_pdr_percent,mean_delay_ms,delay_p50_ms,delay_p95_ms,goodput_kbps,olsr_control_packets,olsr_control_bytes,normalized_overhead,recovery_seconds,next_hop_changes,route_unavailable_samples,signal_transitions,failure_transitions,recoveries,disrupted_node"
printf '%s\n' "${header}" > "${raw_dir}/v2_runs.csv"
task_file="${raw_dir}/v2_tasks.tsv"
: > "${task_file}"
target_file="${raw_dir}/v2_disturbance_targets.tsv"
: > "${target_file}"

# Anchor the disturbance target to the static B0 route for each random run.
probe_task_file="${raw_dir}/v2_target_probe_tasks.txt"
seq 201 230 > "${probe_task_file}"
export ns3_root raw_dir log_dir
xargs -P "${parallel_jobs}" -n 1 bash -c '
  set -euo pipefail
  run="$1"
  log="${log_dir}/target_probe_B_baseline_${run}.log"
  (
    cd "${ns3_root}"
    ./ns3 run --no-build "g4_controlled_experiment --scenario=B --mechanism=baseline --run=${run}" \
      > "${log}" 2>&1
  )
  target=$(tail -n 1 "${log}" | awk -F, '"'"'{print $22}'"'"')
  [[ "${target}" =~ ^[0-9]+$ ]]
  printf "%s\t%s\n" "${run}" "${target}" > "${raw_dir}/target_${run}.row"
' _ < "${probe_task_file}"
for run in $(seq 201 230); do
  cat "${raw_dir}/target_${run}.row" >> "${target_file}"
done

for scenario in A B C D; do
  for mechanism in baseline always_on triggered; do
    for run in $(seq 201 230); do
      target=$(awk -F '\t' -v run="${run}" '$1 == run {print $2}' "${target_file}")
      extra="none"
      if [[ "${mechanism}" == "triggered" ]]; then
        extra="--finalFailureThreshold=20"
      fi
      printf '%s\t%s\t%s\t%s\t%s\n' \
        "${scenario}" "${mechanism}" "${run}" "${target}" "${extra}" >> "${task_file}"
    done
  done
done

xargs -P "${parallel_jobs}" -d '\n' -I '{}' bash -c '
  set -euo pipefail
  IFS=$'"'"'\t'"'"' read -r scenario mechanism run target extra <<< "$1"
  if [[ "${extra}" == "none" ]]; then
    extra=""
  fi
  log="${log_dir}/${scenario}_${mechanism}_${run}.log"
  row="${raw_dir}/${scenario}_${mechanism}_${run}.row"
  cd "${ns3_root}"
  ./ns3 run --no-build "g4_controlled_experiment --scenario=${scenario} --mechanism=${mechanism} --run=${run} --disruptedNode=${target} ${extra}" > "${log}" 2>&1
  tail -n 1 "${log}" > "${row}"
  fields=$(awk -F, '"'"'{print NF}'"'"' "${row}")
  [[ "${fields}" == "22" ]]
' _ '{}' < "${task_file}"

for scenario in A B C D; do
  for mechanism in baseline always_on triggered; do
    for run in $(seq 201 230); do
      cat "${raw_dir}/${scenario}_${mechanism}_${run}.row" >> "${raw_dir}/v2_runs.csv"
    done
  done
done

python3 "${project_root}/scripts/analyze_v2.py" \
  "${raw_dir}/v2_runs.csv" \
  "${project_root}/results/v2" \
  "${project_root}/figures/v2"

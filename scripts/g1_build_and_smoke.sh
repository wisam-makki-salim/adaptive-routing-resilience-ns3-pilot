#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ns3_root="${NS3_ROOT:-${project_root}/../tools/ns-allinone-3.47/ns-3.47}"
export PATH="${HOME}/.local/bin:${PATH}"

python3 "${project_root}/scripts/validate_locked_topology.py"

if [[ ! -x "${ns3_root}/ns3" ]]; then
  echo "ns-3 launcher not found at ${ns3_root}/ns3" >&2
  exit 1
fi

cp "${project_root}/src/g1_smoke.cc" "${ns3_root}/scratch/g1_smoke.cc"
cp "${project_root}/src/g1_olsr_runtime_check.cc" "${ns3_root}/scratch/g1_olsr_runtime_check.cc"
(
  cd "${ns3_root}"
  ./ns3 build g1_smoke
  ./ns3 build g1_olsr_runtime_check
  ./ns3 run "g1_smoke --run=1"
  ./ns3 run g1_olsr_runtime_check
)

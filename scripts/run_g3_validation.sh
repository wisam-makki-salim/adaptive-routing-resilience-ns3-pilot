#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ns3_root="${NS3_ROOT:-${project_root}/../tools/ns-allinone-3.47/ns-3.47}"
export PATH="${HOME}/.local/bin:${PATH}"

cp "${project_root}/src/lra_olsr_controller.h" "${ns3_root}/scratch/lra_olsr_controller.h"
cp "${project_root}/src/g3_adaptive_validation.cc" "${ns3_root}/scratch/g3_adaptive_validation.cc"
(
  cd "${ns3_root}"
  ./ns3 build g3_adaptive_validation
  ./ns3 run g3_adaptive_validation
)

# GitHub Project Description

## Short description

Reproducible ns-3 study of lightweight OLSR adaptation under normal operation, gradual link degradation, and node failure, including paired-seed evaluation, uncertainty analysis, and component ablation.

## Repository introduction

This repository contains an executable research pilot on adaptive and resilient networking. It compares standard OLSR with a lightweight controller that reacts to weak-link and transmission-failure signals by changing routing-control timers. The evaluation uses three controlled scenarios and 20 paired ns-3 runs per comparison. Scripts regenerate raw results, processed tables, bootstrap uncertainty estimates, diagnostic traces, and publication-ready figures.

The locked evaluation does not support a general superiority claim. Adaptive service effects vary across seeds, while overhead rises consistently after triggering. Component ablation identifies accelerated HELLO messaging as the source of the observed adverse tail. A lower failure threshold is retained as an exploratory redesign candidate for independent validation rather than reported as an optimized result.

## Topics

`ns-3` `OLSR` `adaptive-routing` `resilient-networks` `wireless-networks` `network-failures` `reproducible-research` `computer-engineering`

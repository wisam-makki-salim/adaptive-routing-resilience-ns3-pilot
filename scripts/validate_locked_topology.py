#!/usr/bin/env python3
"""Dependency-free validation of the locked 4x4 G0 grid topology."""

from collections import deque

SPACING_M = 60.0
MAX_RANGE_M = 75.0
NODES = {r * 4 + c: (c * SPACING_M, r * SPACING_M) for r in range(4) for c in range(4)}


def adjacent(a, b):
    ax, ay = NODES[a]
    bx, by = NODES[b]
    return (ax - bx) ** 2 + (ay - by) ** 2 <= MAX_RANGE_M**2


GRAPH = {
    node: {other for other in NODES if other != node and adjacent(node, other)}
    for node in NODES
}


def shortest_path(source, target, blocked=frozenset()):
    queue = deque([(source, [source])])
    seen = {source} | set(blocked)
    while queue:
        node, path = queue.popleft()
        if node == target:
            return path
        for neighbor in sorted(GRAPH[node] - seen):
            seen.add(neighbor)
            queue.append((neighbor, path + [neighbor]))
    return None


primary = shortest_path(0, 15)
secondary = shortest_path(0, 15, blocked=set(primary[1:-1]))
after_node_1_failure = shortest_path(0, 15, blocked={1})

assert primary is not None
assert secondary is not None
assert after_node_1_failure is not None
print(f"G1_TOPOLOGY nodes={len(NODES)} edges={sum(map(len, GRAPH.values())) // 2}")
print(f"primary={primary}")
print(f"internally_node_disjoint_alternative={secondary}")
print(f"path_after_node_1_failure={after_node_1_failure}")

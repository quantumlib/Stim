import re
from typing import Dict, Tuple, TypeVar, List, Set, Callable

import networkx as nx

K = TypeVar("K")


def text_diagram_to_networkx_graph(text_diagram: str, *, value_func: Callable[[str], K] = str) -> nx.MultiGraph:
    r"""Converts a text diagram into a networkx multi graph.

    Args:
        text_diagram: An ascii text diagram of the graph, linking nodes together with edges. Edges can be horizontal
            (-), vertical (|), diagonal (/\), crossing (+), or changing direction (*). Nodes can be alphanumeric with
            parentheses. It is assumed that all text is shown with a fixed-width font.
        value_func: An optional transformation to apply to the node text in order to get the node's value. Otherwise
            the node's value is just its text.

    Example:

        >>> import stimzx
        >>> import networkx as nx
        >>> actual = stimzx.text_diagram_to_networkx_graph(r'''
        ...
        ...           A
        ...           |
        ...    NODE1--+--NODE2----------*
        ...           |     |          /
        ...           B     |         /
        ...                 *------NODE4
        ...
        ... ''')
        >>> expected = nx.MultiGraph()
        >>> expected.add_node(0, value='A')
        >>> expected.add_node(1, value='NODE1')
        >>> expected.add_node(2, value='NODE2')
        >>> expected.add_node(3, value='B')
        >>> expected.add_node(4, value='NODE4')
        >>> _ = expected.add_edge(0, 3)
        >>> _ = expected.add_edge(1, 2)
        >>> _ = expected.add_edge(2, 4)
        >>> _ = expected.add_edge(2, 4)
        >>> nx.testing.assert_graphs_equal(actual, expected)

    Returns:
        A networkx multi graph containing the graph from the text diagram. Nodes in the graph are integers (the ordering
        of nodes is in the natural string ordering from left to right then top to bottom in the diagram), and have a
        "value" attribute containing either the node's string from the diagram or else a function of that string if
        value_func was specified.
    """
    char_map = _text_to_char_map(text_diagram)
    node_ids, nodes = _find_nodes(char_map, value_func)
    edges = _find_all_edges(char_map, node_ids)
    result = nx.MultiGraph()
    for k, v in enumerate(nodes):
        result.add_node(k, value=v)
    for a, b in edges:
        result.add_edge(a, b)
    return result


def _text_to_char_map(text: str) -> Dict[complex, str]:
    char_map = {}
    x = 0
    y = 0
    for c in text:
        if c == '\n':
            x = 0
            y += 1
            continue
        if c != ' ':
            char_map[x + 1j*y] = c
        x += 1
    return char_map


DIR_TO_CHARS = {
    -1 - 1j: '\\',
    0 - 1j: '|+',
    1 - 1j: '/',
    -1: '-+',
    1: '-+',
    -1 + 1j: '/',
    1j: '|+',
    1 + 1j: '\\',
}

CHAR_TO_DIR = {
    '\\': 1 + 1j,
    '-': 1,
    '|': 1j,
    '/': -1 + 1j,
}


def _find_all_edges(char_map: Dict[complex, str], terminal_map: Dict[complex, K]) -> List[Tuple[K, K]]:
    edges = []
    already_travelled = set()
    for xy, c in char_map.items():
        x = int(xy.real)
        y = int(xy.imag)
        if xy in terminal_map or xy in already_travelled or c in '*+':
            continue
        already_travelled.add(xy)
        dxy = CHAR_TO_DIR.get(c)
        if dxy is None:
            raise ValueError(f"Character {x+1} ('{c}') in line {y+1} isn't part in a node or an edge")
        n1 = _find_end_of_edge(xy + dxy, dxy, char_map, terminal_map, already_travelled)
        n2 = _find_end_of_edge(xy - dxy, -dxy, char_map, terminal_map, already_travelled)
        edges.append((n2, n1))
    return edges


def _find_end_of_edge(xy: complex, dxy: complex, char_map: Dict[complex, str], terminal_map: Dict[complex, K], already_travelled: Set[complex]):
    while True:
        c = char_map[xy]
        if xy in terminal_map:
            return terminal_map[xy]

        if c != '+':
            if xy in already_travelled:
                raise ValueError("Edge used twice.")
            already_travelled.add(xy)

        next_deltas: List[complex] = []
        if c == '*':
            for dx2 in [-1, 0, 1]:
                for dy2 in [-1, 0, 1]:
                    dxy2 = dx2 + dy2 * 1j
                    c2 = char_map.get(xy + dxy2)
                    if dxy2 != 0 and dxy2 != -dxy and c2 is not None and c2 in DIR_TO_CHARS[dxy2]:
                        next_deltas.append(dxy2)
            if len(next_deltas) != 1:
                raise ValueError(f"Edge junction ('*') at character {int(xy.real)+1}$ in line {int(xy.imag)+1} doesn't have exactly 2 legs.")
            dxy, = next_deltas
        else:
            expected = DIR_TO_CHARS[dxy]
            if c not in expected:
                raise ValueError(f"Dangling edge at character {int(xy.real)+1} in line {int(xy.imag)+1} travelling dx=${int(dxy.real)},dy={int(dxy.imag)}.")
        xy += dxy


def _find_nodes(char_map: Dict[complex, str], value_func: Callable[[str], K]) -> Tuple[Dict[complex, int], List[K]]:
    node_ids = {}
    nodes = []

    node_chars = re.compile("^[a-zA-Z0-9()]$")
    next_node_id = 0

    for xy, lead_char in char_map.items():
        if xy in node_ids:
            continue
        if not node_chars.match(lead_char):
            continue

        n = 0
        nested = 0
        full_name = ''
        while True:
            c = char_map.get(xy + n, ' ')
            if c == ' ' and nested > 0:
                raise ValueError("Label ended before ')' to go with '(' was found.")
            if nested == 0 and not node_chars.match(c):
                break
            full_name += c
            if c == '(':
                nested += 1
            elif c == ')':
                nested -= 1
            n += 1

        nodes.append(value_func(full_name))
        node_id = next_node_id
        next_node_id += 1
        for k in range(n):
            node_ids[xy + k] = node_id

    return node_ids, nodes

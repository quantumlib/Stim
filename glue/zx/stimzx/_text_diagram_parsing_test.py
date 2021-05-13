import networkx as nx
import pytest
from ._text_diagram_parsing import _find_nodes, _text_to_char_map, _find_end_of_edge, _find_all_edges, text_diagram_to_networkx_graph


def test_text_to_char_map():
    assert _text_to_char_map("""
ABC DEF
G    
 HI
    """) == {
        0 + 1j: 'A',
        1 + 1j: 'B',
        2 + 1j: 'C',
        4 + 1j: 'D',
        5 + 1j: 'E',
        6 + 1j: 'F',
        0 + 2j: 'G',
        1 + 3j: 'H',
        2 + 3j: 'I',
    }


def test_find_nodes():
    assert _find_nodes(_text_to_char_map(''), lambda e: e) == ({}, [])
    with pytest.raises(ValueError, match="base 10"):
        _find_nodes(_text_to_char_map('NOTANINT'), int)
    with pytest.raises(ValueError, match=r"ended before '\)'"):
        _find_nodes(_text_to_char_map('X(run_off'), str)
    assert _find_nodes(_text_to_char_map('X'), str) == (
        {
            0j: 0,
        },
        ['X'],
    )
    assert _find_nodes(_text_to_char_map('\n   X'), str) == (
        {
            3 + 1j: 0,
        },
        ['X'],
    )
    assert _find_nodes(_text_to_char_map('X(pi)'), str) == (
        {
            0: 0,
            1: 0,
            2: 0,
            3: 0,
            4: 0,
        },
        ['X(pi)'],
    )
    assert _find_nodes(_text_to_char_map('X--Z'), str) == (
        {
            0: 0,
            3: 1,
        },
        ['X', 'Z'],
    )
    assert _find_nodes(_text_to_char_map("""
X--*
  /
 Z
"""), str) == (
        {
            1j: 0,
            1 + 3j: 1,
        },
        ['X', 'Z'],
    )
    assert _find_nodes(_text_to_char_map("""
X(pi)--Z
"""), str) == (
        {
            0 + 1j: 0,
            1 + 1j: 0,
            2 + 1j: 0,
            3 + 1j: 0,
            4 + 1j: 0,
            7 + 1j: 1,
        },
        ["X(pi)", "Z"],
    )


def test_find_end_of_edge():
    c = _text_to_char_map(r"""
1--------*
          \    2      |
     5     \      *--++-*
            *-----+-* |/
                  | | /
                  2 |/
                    *
    """)
    terminal = {1: 'ONE', 18 + 6j: 'TWO'}
    seen = set()
    assert _find_end_of_edge(1 + 1j, 1, c, terminal, seen) == 'TWO'
    assert len(seen) == 31


def test_find_all_edges():
    c = _text_to_char_map(r"""
X---Z      H----X(pi/2)
          /
       Z(pi/2)
    """)
    node_ids, _ = _find_nodes(c, str)
    assert _find_all_edges(c, node_ids) == [
        (0, 1),
        (2, 3),
        (2, 4),
    ]


def test_from_text_diagram():
    actual = text_diagram_to_networkx_graph("""
in---Z---H---------out
     |
in---X---Z(-pi/2)---out
    """)
    expected = nx.MultiGraph()
    expected.add_node(0, value='in'),
    expected.add_node(1, value='Z'),
    expected.add_node(2, value='H'),
    expected.add_node(3, value='out'),
    expected.add_node(4, value='in'),
    expected.add_node(5, value='X'),
    expected.add_node(6, value='Z(-pi/2)'),
    expected.add_node(7, value='out'),
    expected.add_edge(0, 1)
    expected.add_edge(1, 2)
    expected.add_edge(2, 3)
    expected.add_edge(1, 5)
    expected.add_edge(4, 5)
    expected.add_edge(5, 6)
    expected.add_edge(6, 7)
    nx.testing.assert_graphs_equal(actual, expected)

    actual = text_diagram_to_networkx_graph("""
        Z-*
        | |
        X-*
    """)
    expected = nx.MultiGraph()
    expected.add_node(0, value='Z')
    expected.add_node(1, value='X')
    expected.add_edge(0, 1)
    expected.add_edge(0, 1)
    nx.testing.assert_graphs_equal(actual, expected)

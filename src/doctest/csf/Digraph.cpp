#include <test/csf/Digraph.h>

#include <doctest/doctest.h>

#include <sstream>
#include <string>
#include <vector>

using namespace xrpl::test::csf;

TEST_SUITE_BEGIN("Digraph");

TEST_CASE("Digraph basic operations")
{
    using Graph = Digraph<char, std::string>;
    Graph graph;

    CHECK(!graph.connected('a', 'b'));
    CHECK(!graph.edge('a', 'b'));
    CHECK(!graph.disconnect('a', 'b'));

    CHECK(graph.connect('a', 'b', "foobar"));
    CHECK(graph.connected('a', 'b'));
    CHECK(*graph.edge('a', 'b') == "foobar");

    CHECK(!graph.connect('a', 'b', "repeat"));
    CHECK(graph.disconnect('a', 'b'));
    CHECK(graph.connect('a', 'b', "repeat"));
    CHECK(graph.connected('a', 'b'));
    CHECK(*graph.edge('a', 'b') == "repeat");

    CHECK(graph.connect('a', 'c', "tree"));

    {
        std::vector<std::tuple<char, char, std::string>> edges;

        for (auto const& edge : graph.outEdges('a'))
        {
            edges.emplace_back(edge.source, edge.target, edge.data);
        }

        std::vector<std::tuple<char, char, std::string>> expected;
        expected.emplace_back('a', 'b', "repeat");
        expected.emplace_back('a', 'c', "tree");
        CHECK(edges == expected);
        CHECK(graph.outDegree('a') == expected.size());
    }

    CHECK(graph.outEdges('r').size() == 0);
    CHECK(graph.outDegree('r') == 0);
    CHECK(graph.outDegree('c') == 0);

    // only 'a' has out edges
    CHECK(graph.outVertices().size() == 1);
    std::vector<char> expected = {'b', 'c'};

    CHECK((graph.outVertices('a') == expected));
    CHECK(graph.outVertices('b').size() == 0);
    CHECK(graph.outVertices('c').size() == 0);
    CHECK(graph.outVertices('r').size() == 0);

    std::stringstream ss;
    graph.saveDot(ss, [](char v) { return v; });
    std::string expectedDot =
        "digraph {\n"
        "a -> b;\n"
        "a -> c;\n"
        "}\n";
    CHECK(ss.str() == expectedDot);
}

TEST_SUITE_END();


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

    CHECK_FALSE(graph.connected('a', 'b'));
    CHECK_FALSE(graph.edge('a', 'b'));
    CHECK_FALSE(graph.disconnect('a', 'b'));

    CHECK_UNARY(graph.connect('a', 'b', "foobar"));
    CHECK_UNARY(graph.connected('a', 'b'));
    CHECK_EQ(*graph.edge('a', 'b'), "foobar");

    CHECK_FALSE(graph.connect('a', 'b', "repeat"));
    CHECK_UNARY(graph.disconnect('a', 'b'));
    CHECK_UNARY(graph.connect('a', 'b', "repeat"));
    CHECK_UNARY(graph.connected('a', 'b'));
    CHECK_EQ(*graph.edge('a', 'b'), "repeat");

    CHECK_UNARY(graph.connect('a', 'c', "tree"));

    {
        std::vector<std::tuple<char, char, std::string>> edges;

        for (auto const& edge : graph.outEdges('a'))
        {
            edges.emplace_back(edge.source, edge.target, edge.data);
        }

        std::vector<std::tuple<char, char, std::string>> expected;
        expected.emplace_back('a', 'b', "repeat");
        expected.emplace_back('a', 'c', "tree");
        CHECK_EQ(edges, expected);
        CHECK_EQ(graph.outDegree('a'), expected.size());
    }

    CHECK_EQ(graph.outEdges('r').size(), 0);
    CHECK_EQ(graph.outDegree('r'), 0);
    CHECK_EQ(graph.outDegree('c'), 0);

    // only 'a' has out edges
    CHECK_EQ(graph.outVertices().size(), 1);
    std::vector<char> expected = {'b', 'c'};

    CHECK_EQ(graph.outVertices('a'), expected);
    CHECK_EQ(graph.outVertices('b').size(), 0);
    CHECK_EQ(graph.outVertices('c').size(), 0);
    CHECK_EQ(graph.outVertices('r').size(), 0);

    std::stringstream ss;
    graph.saveDot(ss, [](char v) { return v; });
    std::string expectedDot =
        "digraph {\n"
        "a -> b;\n"
        "a -> c;\n"
        "}\n";
    CHECK_EQ(ss.str(), expectedDot);
}

TEST_SUITE_END();

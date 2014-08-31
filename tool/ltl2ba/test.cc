/* Test ltl_graph_t */

#include <iostream>

#include "formul.hh"
#include "ltl.hh"
#include "alt_graph.hh"

using namespace std;
using namespace divine;

int main(void)
{
	LTL_formul_t F;
	LTL_parse_t L;
	ALT_graph_t G;

	L.nacti();
	if (L.syntax_check(F)) {
		G.create_graph(F);
		G.transform_vwaa();
		G.simplify_GBA();
		G.degeneralize();
		G.vypis_BA();
	} else {
		cout << "Nejde o LTL formuli" << endl;
	}
}

/*
*  Transformation LTL formulae to Buchi automata
*
*  Main program - variant 2
*
*  Milan Jaros - xjaros1@fi.muni.cz
*
*/

#include <sstream>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <string>
#include <iterator>
#include <list>
#include <map>
#include <cctype>
#include <cstdlib>

#include "buchi_lang.hh"
#include "formul.hh"
#include "ltl.hh"
#include "alt_graph.hh"
#include "opt_buchi.hh"
#include "DBA_graph.hh"
#include "support_dve.hh"

using namespace divine;
using namespace std;

void run_usage()
{
#if BUCHI_LANG == BUCHI_LANG_CZ
	cout << "Pouziti:" << endl;
	cout << "-h - vypis napovedy" << endl;
	cout << "-r (0, 1, 2, 3, 4) - aplikace prepisovacich pravidel:" <<
		endl;
	cout << "\t0 - neaplikovat zadna pravidla" << endl;
	cout << "\t1 - aplikovat sadu pravidel 1" << endl;
	cout << "\t2 - aplikovat sadu pravidel 2" << endl;
	cout << "\t3 - aplikovat obe sady v poradi 1, 2 (defaultne)" <<
		endl;
	cout << "\t4 - aplikovat obe sady v poradi 2, 1" << endl;
	cout << "-g - negovat zadanou LTL formuli" << endl;
	cout << "-o (0, 1) - format vystupu "
		"(neni-li pouzit prepinac '-f')" << endl;
	cout << "\t0 - format DVE (defaultne)" << endl;
	cout << "\t1 - format s labely na hranach" << endl;
	cout << "-t - jednoduchy vypis" << endl;
	cout << "-l soubor - nacte LTL formule ze souboru" << endl;
	cout << "-f soubor[.dve] - vystup ulozi do souboru "
		"'soubor.prop.dve'" << endl;
	cout << "-O stupen - optimalizace automatu" << endl;
	cout << "-s (0, 1, 2) - optimalizace GBA/BA" << endl;
        cout << "-p - (semi)determinizace pro pravdepodobnostni Model Checking" << endl;
	cout << "\t0 - zadna optimalizace" << endl;
	cout << "\t1 - zjednoduseni po sestrojeni automatu" <<
		" - defaultne" << endl;
	cout << "\t2 - zjednoduseni behem vypoctu (on the fly)" <<
		//" - defaultne" <<
		endl;

#elif BUCHI_LANG == BUCHI_LANG_EN
	cout << "Usage:" << endl;
	cout << "-h - print help" << endl;
	cout << "-r (0, 1, 2, 3, 4) - apply rewriting rules:" <<
		endl;
	cout << "\t0 - do not apply any rules" << endl;
	cout << "\t1 - apply first set of rules" << endl;
	cout << "\t2 - apply second set of rules" << endl;
	cout << "\t3 - apply both sets of rules in sequence 1, 2 "
		"(default)" << endl;
	cout << "\t4 - apply both sets of rules in sequence 2, 1" << endl;
	cout << "-g - negate LTL formula before translation" << endl;
	cout << "-o (0, 1) - output format "
		"(when option '-f' were not used)" << endl;
	cout << "\t0 - DiVinE format (defaultne)" << endl;
	cout << "\t1 - format with labeled transitions" << endl;
	cout << "-t - strict output" << endl;
	cout << "-l file - read LTL formula from file" << endl;
	cout << "-f file[.dve] - write output to DiVinE source file "
		"'file.prop.dve'" << endl;
	cout << "-O level - optimizing Buchi automaton" << endl;
	cout << "-s (0, 1, 2) - simplification GBA/BA during computation";
	cout << endl;
	cout << "\t0 - do not simplify" << endl;
	cout << "\t1 - simplify after construct automaton - default";
	cout << endl;
	cout << "\t2 - simplification during computation (on the fly)" <<
		endl;
        cout << "-p - (semi)determinization for propabilistic Model Checking" << endl;
#endif
}

void run_help()
{
	run_usage();

	cout << endl;
#if BUCHI_LANG == BUCHI_LANG_CZ
	cout << "Syntaxe formule LTL:" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
	cout << "Syntax of LTL formula:" << endl;
#endif
	cout << "  F :== un_op F | F bin_op F | (F) | term" << endl;
	cout << "  un_op :== '!' (negace)" << endl;
	cout << "        :== 'X' | 'O' (next)" << endl;
	cout << "        :== 'F' | '<>' (true U 'argument')" << endl;
	cout << "        :== 'G' | '[]' (!F!'argument')" << endl;
	cout << "  bin_op :== '&&' | '*' (and)" << endl;
	cout << "         :== '||' | '+' (or)" << endl;
#if BUCHI_LANG == BUCHI_LANG_CZ
	cout << "         :== '->' (implikace)" << endl;
	cout << "         :== '<->' (ekvivalence)" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
	cout << "         :== '->' (implication)" << endl;
	cout << "         :== '<->' (equivalention)" << endl;
#endif
	cout << "         :== '^' (xor)" << endl;
	cout << "         :== 'U' (until)" << endl;
	cout << "         :== 'V' | 'R' (release)" << endl;
	cout << "         :== 'W' (weak until)" << endl;
	cout << "  term :== 'true' | 'false'" << endl;
	cout << "       :== str" << endl;
#if BUCHI_LANG == BUCHI_LANG_CZ
	cout << "           str je retezec, slozeny z malych pismen," <<
		"cislic ";
	cout << endl;
	cout << "           a znaku \'_\', zacinajici pismenem" <<
		" nebo znakem \'_\'" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
	cout << "           str is string contains low characters, " <<
		"numbers ";
	cout << endl;
	cout << "           and character \'_\', begining character a - z"
		" or \'_\'" << endl;
#endif
}

void copy_graph(const ALT_graph_t& aG, BA_opt_graph_t& oG)
{
	list<ALT_ba_node_t*> node_list, init_nodes, accept_nodes;
	list<ALT_ba_node_t*>::const_iterator n_b, n_e, n_i;
	list<ALT_ba_trans_t>::const_iterator t_b, t_e, t_i;
	KS_BA_node_t *p_N;

	node_list = aG.get_all_BA_nodes();
	init_nodes = aG.get_init_BA_nodes();
	accept_nodes = aG.get_accept_BA_nodes();

	oG.clear();

	n_b = node_list.begin(); n_e = node_list.end();
	for (n_i = n_b; n_i != n_e; n_i++) {
		p_N = oG.add_node((*n_i)->name);
		t_b = (*n_i)->adj.begin(); t_e = (*n_i)->adj.end();
		for (t_i = t_b; t_i != t_e; t_i++) {
			oG.add_trans(p_N, t_i->target->name,
				t_i->t_label);
		}
	}

	n_b = init_nodes.begin(); n_e = init_nodes.end();
	for (n_i = n_b; n_i != n_e; n_i++) {
		oG.set_initial((*n_i)->name);
	}

	n_b = accept_nodes.begin(); n_e = accept_nodes.end();
	for (n_i = n_b; n_i != n_e; n_i++) {
		oG.set_accept((*n_i)->name);
	}
}

int main(int argc, char **argv)
{
        LTL_formul_t F, F1;
	LTL_parse_t L;
	ALT_graph_t G;
	BA_opt_graph_t oG, oG1 ,oG2;

	int c, i;
	bool negace = false, out_props, prob = false;
	bool strict_output = false;
	bool output_pom = false;
	int rewrite_set = 3, optimize = -1, out_format = 0;
	int optimize_gba = 1;
	string jmeno_ltl = "", jmeno_dve = "", one_ltl = "";

	list<LTL_formul_t> L_formul;
	list<map<string, string> > L_prop;
	list<LTL_formul_t>::iterator lf_b, lf_e, lf_i;
	list<map<string, string> >::iterator pr_b, pr_e, pr_i, pr_pom;

	while ((c = getopt(argc, argv,
		"hr:-gto:-O:-l:-f:-s:-dpn:-")) != -1) {
		switch (c) {
		case 'h': // napoveda
			run_help();
			exit(0);
		case 'r': // prepisovani
			rewrite_set = atoi(optarg);
			if (rewrite_set < 0) rewrite_set = 0;
			if (rewrite_set > 3) rewrite_set = 3;
			break;
		case 'g': // negovat formuli
			negace = true;
			break;
		case 's': // optimalizace GBA/BA
			optimize_gba = atoi(optarg);
			if (optimize_gba < 0) optimize_gba = 0;
			if (optimize_gba > 2) optimize_gba = 2;
			break;
		case 'O': // optimalizace
			optimize = atoi(optarg);
			break;
		case 'l': // nacist LTL formule ze souboru
			jmeno_ltl = optarg;
			break;
		case 'n': // nacteni (jedine) LTL formule ze souboru
			one_ltl = optarg;
			break;
		case 'f': // dve soubor
			jmeno_dve = optarg;
			break;
		case 'o': // format vystupu
			out_format = atoi(optarg);
			if (out_format < 0) out_format = 0;
			if (out_format > 1) out_format = 1;
			break;
		case 't': // jednoduchy vypis
			strict_output = true;
			break;
		case 'd': // pomocne vypisy
			output_pom = true;
			break;
		case 'p': //pro pravdepodobnostni MCH
			prob = true;
			break;
                case '?':
			run_usage();
			exit(9);
			break;
		}
	}

	if ((jmeno_ltl != "") && (one_ltl != "")) {
#if BUCHI_LANG == BUCHI_LANG_CZ
		cerr << "Lze pouzit nejvyse jeden z prepinacu '-l', '-n'";
		cerr << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
		cerr << "You can use only one of option '-l', '-n'" <<
		endl;
#endif
		return(2);
	}
	if (jmeno_ltl != "") {
		list<LTL_parse_t> L_ltl;
		list<LTL_parse_t>::iterator l_b, l_e, l_i;
		bool b;
		fstream(fr);
		fr.open(jmeno_ltl.c_str(), ios::in);
		if (!fr) {
#if BUCHI_LANG == BUCHI_LANG_CZ
			cerr << "Soubor '" << jmeno_ltl << "' neexistuje";
			cerr << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
			cerr << "File '" << jmeno_ltl <<
			"' doesn't exist" << endl;
#endif
			return 2;
		}

		b = read_ltl_file(fr, L_ltl, L_prop);

		fr.close();

		if (!b) {
#if BUCHI_LANG == BUCHI_LANG_CZ
			cerr << "Soubor '" << jmeno_ltl << "' nema "
			"spravny format" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
			cerr << "File '" << jmeno_ltl <<
			"' doesn't have correct format" << endl;
#endif
			return 3;
		}
		l_b = L_ltl.begin(); l_e = L_ltl.end();
		pr_b = L_prop.begin(); pr_e = L_prop.end();
		for (l_i = l_b, pr_i = pr_b; l_i != l_e; l_i++) {
			if (l_i->syntax_check(F)) {
				L_formul.push_back(F);
				pr_i++;
			} else {
#if BUCHI_LANG == BUCHI_LANG_CZ
				cerr << "Nejde o LTL formuli: " <<
#elif BUCHI_LANG == BUCHI_LANG_EN
				cerr << "This is not LTL formula: " <<
#endif
				(*l_i) << endl;
				pr_pom = pr_i; pr_i++;
				L_prop.erase(pr_pom);
			}
		}
		out_props = true;
	} else {
		if (one_ltl != "") {
			fstream(fr);

			fr.open(one_ltl.c_str(), ios::in);
			if (!fr) {
#if BUCHI_LANG == BUCHI_LANG_CZ
				cerr << "Soubor '" << one_ltl <<
				"' neexistuje" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
				cerr << "File '" << jmeno_ltl <<  
				"' doesn't exist" << endl;
#endif
				return(2);
			}

			L.nacti(fr, cout);
			fr.close();
		} else {
			if (strict_output) {
				cin >> L;
			} else {
				L.nacti();
			}
		}

		if (L.syntax_check(F)) {
			L_formul.push_back(F);
		} else {
#if BUCHI_LANG == BUCHI_LANG_CZ
			cerr << "Toto neni LTL formule" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
			cerr << "This is not LTL formula" << endl;
#endif
			return 1;
		}
		out_props = false;
	}
        
        
	lf_b = L_formul.begin(); lf_e = L_formul.end();
	pr_b = L_prop.begin(); pr_e = L_prop.end();
	i = 1;
	for (lf_i = lf_b, pr_i = pr_b; lf_i != lf_e; lf_i++, i++) {
		reg_ltl_at_props(0);
		if (negace) F = lf_i->negace();
		else F = *lf_i;

		if (!strict_output && !out_props) {
#if BUCHI_LANG == BUCHI_LANG_CZ
			cout << "Formule v normalni forme:" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
			cout << "Formula in positive normal form:" <<
			endl;
#endif
			cout << F << endl << endl;
		}

		switch (rewrite_set) {
		case 1: F.rewrite(F1); F = F1;
			break;
		case 2: F.rewrite2(F1); F = F1;
			break;
		case 3: F.rewrite(F1); F1.rewrite2(F);
			break;
		case 4: F.rewrite2(F1); F1.rewrite(F);
			break;
		}

		if (!strict_output && !out_props && (rewrite_set != 0)) {
#if BUCHI_LANG == BUCHI_LANG_CZ
			cout << "Prepsana formule:" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
			cout << "Rewriten formula:" << endl;
#endif
			cout << F << endl << endl;
		}

		ostringstream oss;
		oss <<F;

		if (oss.str() == "false")
		  {
		    optimize = 0;
		    optimize_gba = 0;
		    if (!strict_output && !out_props)
		      {
#if BUCHI_LANG == BUCHI_LANG_CZ
			cout << "Nebudu provadet optimalizace."<<endl<<endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
			cout <<"Optimizations turned off."<<endl<<endl;
#endif
		      }
		  }

		if (optimize_gba == 2) G.simplify_on_the_fly();
		
		G.create_graph(F);
		if (output_pom) {
			cout << "VWAA" << endl;
			G.vypis_vwaa(); cout << endl;
		}

		G.transform_vwaa();

		if (output_pom) {
			cout << "GBA" << endl;
			G.vypis_GBA(); cout << endl;
		}

		if (optimize_gba == 1) {
			G.simplify_GBA();
			if (output_pom) {
				cout << "optimalizovany GBA" << endl;
				G.vypis_GBA(); cout << endl;
			}
		}

		G.degeneralize();

		if (output_pom) {
			cout << "BA" << endl;
			G.vypis_BA(); cout << endl;
		}

		if (optimize_gba >= 1) G.simplify_BA();

		copy_graph(G, oG);
		oG.to_one_initial();

		oG.optimize(oG1, optimize);
                
                
                /*cout<<"----------------------"<<endl;
		cout<<"Nedeterministicky"<<endl;
                oG1.vypis(cout,false);
	        if (oG1.get_aut_type() == BA_WEAK) cout<<"Je weak"<<endl;
                else cout<<"Neni weak"<<endl;
                if (oG1.is_semideterministic()) cout<<"Je semideterministick"<<endl;
		else cout<<"Neni semideterministick"<<endl;  
                cout<<"----------------------"<<endl;*/

                if (prob)
			{
                         if (!oG1.is_semideterministic())
                         { 
			  //cout<<"----------------------"<<endl
                          //cout<<"Semideterminizace"<<endl;
			  DBA_graph_t DG;
                          DG.semideterminization(oG1);
                          //DG.vypis(cout,false);
		          oG1=DG.transform();
			  //oG1.vypis(cout,false);
		          //if (oG1.get_aut_type() == BA_WEAK) cout<<"Vysledek je weak"<<endl;
                          //else cout<<"Vysledek neni weak"<<endl;
                          //cout<<"----------------------"<<endl;
                         }
			}
		if (jmeno_dve != "") {
			
			if (out_props) {
				reg_ltl_at_props(&(*pr_i));
				output_to_file(oG1, jmeno_dve, i);
#if BUCHI_LANG == BUCHI_LANG_CZ
			cerr << "Zapsana formule c. " << i << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
			cerr << "writing formula no. " << i << endl;
#endif
			} else {
				output_to_file(oG1, jmeno_dve);
			}
		} else {
			if (!out_props && !strict_output) {
#if BUCHI_LANG == BUCHI_LANG_CZ
				cout << "Buchiho automat:" << endl;
#elif BUCHI_LANG == BUCHI_LANG_EN
				cout << "Buchi automaton:" << endl;
#endif
			}
			switch (out_format) {
			case 0: // DVE format
				
				if (out_props) reg_ltl_at_props(&(*pr_i));
				output(oG1, cout);
				
				break;
			case 1: // format s labely na hranach
				oG1.vypis(strict_output);
				if (strict_output) cout << "# ";
				switch (oG1.get_aut_type()) {
				case BA_STRONG:
					cout << "strong"; break;
				case BA_WEAK:
					cout << "weak"; break;
				case BA_TERMINAL:
					cout << "terminal"; break;
				}
				cout << endl;
				break;
			}
		}

		if (out_props) pr_i++;
	}
}

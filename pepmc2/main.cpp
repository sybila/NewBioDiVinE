#include "state_space.hpp"
#include "transitional_state_space.hpp"
#include "paramset.hpp"
#include "buchi_automaton.hpp"

#include "../src/system/bio/parser/parse.cc"
#include "../src/system/bio/scanner/lex.cc"
#include "../src/system/bio/data_model/Model.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <iomanip>
#include <deque>
#include <queue>

#include "succ.hpp"

#ifdef __GNUC__

#include <sys/time.h>

long long my_clock()
{
	timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#else

#include <windows.h>

long long my_clock()
{
	return GetTickCount();
}

#endif

typedef unsigned int uint;


template <typename T>
class single_elem_enum
{
public:
	explicit single_elem_enum(T const & value)
		: m_valid(true), m_value(value)
	{
	}

	bool valid() const
	{
		return m_valid;
	}

	void next()
	{
		m_valid = false;
	}

	T get() const
	{
		return m_value;
	}

private:
	T m_value;
	bool m_valid;
};



class default_pmc_visitor
{
public:
	long long time;
	bool show_counterexamples;
	bool show_base_coloring;
	bool verbose;

	std::size_t reachable_vertices;

	explicit default_pmc_visitor(bool show_counterexamples, bool show_base_coloring, bool verbose)
		: show_counterexamples(show_counterexamples), show_base_coloring(show_base_coloring), verbose(verbose), reachable_vertices(0)
	{
	}

	//shows time between steps
	void progress(int step, int max)
	{
		long long value = 0;
		if (step == 0)
		{
			time = my_clock();
		}
		else
		{
			long long new_time = my_clock();
			value = new_time - time;
			time = new_time;
		}

		std::cout << step << "/" << max << ": " << value << "ms        \r" << std::flush;
	}

	//shows bfs levels
	void succ_bfs_levels(std::size_t levels)
	{
		if (verbose)
			std::cout << "bfs levels: " << levels << std::endl;
	}

	//counts reachable vertices and shows base coloring 
	template <typename Coloring>
	void base_coloring(Coloring const & c)
	{
		if (verbose)
		{
			for (auto ci = c.begin(); ci != c.end(); ++ci)
				if (!ci->second.empty())
					++reachable_vertices;
		}

		if (!show_base_coloring)
			return;

		for (auto ci = c.begin(); ci != c.end(); ++ci)
			std::cout << ci->first << ": " << ci->second << std::endl;
	}

	//shows counterexample self loop
	template <typename Paramset, typename Vertex>
	void counter_example_self_loop(Vertex const & witness, Paramset const & p) const
	{
		if (!show_counterexamples)
			return;

		std::cout << "counterexample space " << p << std::endl;
		std::cout << witness << " <- " << witness << std::endl;
	}

	//shows counterexample
	template <typename Coloring, typename Vertex>
	void counter_example(Coloring const & c, Coloring const & nested_c, Vertex const & witness) const
	{
		typedef typename Coloring::value_type::second_type Paramset;

		if (!show_counterexamples)
			return;

		std::cout << "counterexample space " << nested_c.find(witness)->second << std::endl;

		Vertex current = witness;
		std::size_t region = 0;
		for (;;)
		{
			Paramset const & p = nested_c.find(current)->second;
			typename Paramset::region_t const & r = p.regions().at(region);

			std::cout << current << "<-" << r.tag << std::endl;

			current = r.tag;

			if (current == witness)
				break;

			Paramset const & new_p = nested_c.find(current)->second;
			for (std::size_t i = 0; i < new_p.regions().size(); ++i)
			{
				typename Paramset::region_t const & new_r = new_p.regions().at(i);

				bool subset = true;
				for (std::size_t j = 0; j < new_r.box.size(); ++j)
				{
					if (r.box[j].first < new_r.box[j].first
						|| r.box[j].second > new_r.box[j].second)
					{
						subset = false;
						break;
					}
				}

				if (subset)
				{
					region = i;
					break;
				}
			}
		}
	}
};

struct no_crop
{
	template <typename Vertex, typename Paramset>
	void operator()(Vertex const &, Paramset &)
	{
	}
};

/*
template <typename Structure, typename Paramset, typename Visitor>
Paramset naive(Structure const & s, Paramset const & pp, Visitor visitor)
{
	Paramset res;

	visitor.progress(0, 0);

	std::map<typename Structure::vertex_descriptor, Paramset> c
		= succ(s, typename Structure::init_enumerator(s), pp, no_crop(), visitor);

	int final_count = 0;
	for (auto ci = c.begin(); ci != c.end(); ++ci)
	{
		if (s.final(ci->first))
			++final_count;
	}

	int final_index = 0;
	for (auto ci = c.begin(); ci != c.end(); ++ci)
	{
		if (s.final(ci->first))
		{
			visitor.progress(++final_index, final_count);
			std::map<typename Structure::vertex_descriptor, Paramset> nested_c = succ(s, single_elem_enum<typename Structure::vertex_descriptor>(ci->first), ci->second, no_crop());
			Paramset const & vp = nested_c[ci->first];
			if (res.set_union(vp))
				visitor.counter_example(c, nested_c, ci->first);
		}
	}

	return res;
}
*/

template <typename Coloring>
struct allowed_crop
{
	explicit allowed_crop(Coloring const & c)
		: m_c(c)
	{
	}

	template <typename Vertex, typename Paramset>
	void operator()(Vertex const & v, Paramset & p)
	{
		typename Coloring::const_iterator ci = m_c.find(v);
		if (ci == m_c.end())
			p.clear();
		else
			p.set_intersection(ci->second);
	}

private:
	Coloring const & m_c;
};

template <typename Coloring>
struct forbidden_crop
{
	explicit forbidden_crop(Coloring const & c)
		: m_c(c)
	{
	}

	template <typename Vertex, typename Paramset>
	void operator()(Vertex const & v, Paramset & p)
	{
		typename Coloring::const_iterator ci = m_c.find(v);
		if (ci != m_c.end())
			p.set_difference(ci->second);
	}

private:
	Coloring const & m_c;
};

template <typename Structure, typename Paramset, typename Visitor, typename Succ>
Paramset nonnaive(Structure const & s, Paramset const & pp, Visitor & visitor, Succ succ)
{
	Paramset res;

	visitor.progress(0, 0);
	typedef std::map<typename Structure::vertex_descriptor, Paramset> coloring_type;
	coloring_type c = succ(s, typename Structure::init_enumerator(s), pp, no_crop(), visitor);
	visitor.base_coloring(c);

	int final_count = 0;
	for (auto ci = c.begin(); ci != c.end(); ++ci)
	{
		if (!ci->second.empty() && s.final(ci->first))
			++final_count;
	}

	coloring_type forbidden_coloring;

	int final_index = 0;
	for (auto ci = c.begin(); ci != c.end(); ++ci)
	{
		if (!ci->second.empty() && s.final(ci->first))
		{
			visitor.progress(++final_index, final_count);

			Paramset p = ci->second;
			s.self_loop(ci->first, p);
			if (res.set_union(p))
				visitor.counter_example_self_loop(ci->first, p);

			p = ci->second;
			p.set_difference(res);
			std::map<typename Structure::vertex_descriptor, Paramset> nested_c
				= succ(s, single_elem_enum<typename Structure::vertex_descriptor>(ci->first), p,
				forbidden_crop<coloring_type>(forbidden_coloring), visitor);

			Paramset const & vp = nested_c[ci->first];
			if (res.set_union(vp))
				visitor.counter_example(c, nested_c, ci->first);

			p = pp;
			p.set_difference(res);
			if (p.empty())
				break;

			// FIXME: the second succ run should be over the transposed graph
#if 0
			std::map<typename Structure::vertex_descriptor, Paramset> scc_c = succ(s, single_elem_enum<typename Structure::vertex_descriptor>(ci->first), vp, allowed_crop<coloring_type>(nested_c));
			for (auto ci = scc_c.begin(); ci != scc_c.end(); ++ci)
				forbidden_coloring[ci->first].set_union(std::move(ci->second));
#endif
		}
	}

	return res;
}


int main(int argc, char * argv[])
{
	typedef state_space<double> ss_t;
	typedef buchi_automaton<ss_t::proposition> ba_t;
	std::map<std::string, ba_t> bas;

	ss_t ss2;
	std::vector<std::string> varnames;

	struct schedule_entry
	{
		std::string succ_next;
		std::string succ_message;
		std::string fail_next;
		std::string fail_message;
	};

	std::map<std::string, schedule_entry> schedule;

	transitional_state_space<double> ss3(ss2);
	typedef ba_sync_product<transitional_state_space<double>::proposition, transitional_state_space<double> > sba_t;

	typedef paramset<double, sba_t::vertex_descriptor> pp_t;
	pp_t pp3;
	
	bool useFastLinearAproximation = false;
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] == "-f")
			useFastLinearAproximation = true;
	}
	
	// file opening
	char *file_name = argv[argc - 1];
	if (!strncmp(file_name, "bio", 3)) {
		std::cout << "Bio file name error." << std::endl;
	}

	std::ifstream fin(file_name);	
	std::ifstream fin2(file_name);

	
	if (!fin || !fin2) {
		std::cout << "Bio file could not be read." << std::endl;
	}
	
	std::string l;

	Parser parser(fin);
	
	parser.parse();
	Model<double> &storage = parser.returnStorage();

	storage.RunAbstraction(useFastLinearAproximation);

	fin.close();
	std::vector<std::pair<double, double> > param_ranges(storage.getParamRanges());
	




	ss2.initialize(fin2, storage, param_ranges, varnames, bas);

	pp3.add_region(param_ranges.begin(), param_ranges.end());

	
	std::size_t thread_count = 1;
	bool show_counterexamples = false;
	bool show_base_coloring = false;
	bool verbose = false;


	// Parse arguments
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];

		if (arg == "-j")
			thread_count = boost::lexical_cast<std::size_t>(argv[++i]);

		if (arg == "-c")
			show_counterexamples = true;

		if (arg == "-b")
			show_base_coloring = true;

		if (arg == "-s")
			std::cout << ss3 << std::endl;

		if (arg == "-V")
			verbose = true;

//		if (arg == "-u")
//			ss2.m_final.clear();

		if (arg == "-t")
		{
			std::cout << "thresholds:" << std::endl;
			for (std::size_t i = 0; i < ss2.m_thresholds.size(); ++i)
			{
				std::cout << storage.getVariable(i) << ":";
				for (std::size_t j = 0; j < ss2.m_thresholds[i].size(); ++j)
				{
					if (j > 0)
						std::cout << ',';
					std::cout << ss2.m_thresholds[i][j];
				}
				std::cout << std::endl;
			}
		}
/*
		if (arg == "parse")
			return 0;

		if (arg == "plot")
		{
			float param[] = { boost::lexical_cast<float>(argv[i+1]) };

			float ideal_len = 0.1f;
			float step = 0.5f;

			if (i+2 < argc)
				ideal_len = boost::lexical_cast<float>(argv[i+2]);
			if (i+3 < argc)
				step = boost::lexical_cast<float>(argv[i+3]);

			double xmin = ss2.m_thresholds[0].front();
			double xmax = ss2.m_thresholds[0].back();
			double ymin = ss2.m_thresholds[1].front();
			double ymax = ss2.m_thresholds[1].back();

			if (i+4 < argc)
				xmin = boost::lexical_cast<float>(argv[i+4]);
			if (i+5 < argc)
				xmax = boost::lexical_cast<float>(argv[i+5]);
			if (i+6 < argc)
				ymin = boost::lexical_cast<float>(argv[i+6]);
			if (i+7 < argc)
				ymax = boost::lexical_cast<float>(argv[i+7]);

			double ideal_leny = ideal_len / (xmax - xmin) * (ymax - ymin);
			double stepy = step / (xmax - xmin) * (ymax - ymin);
			const float null_thr = ideal_len / 5;

			std::cout << "set nokey\n"
				"plot '-' using 1:2:3:4 with vectors\n";
			for (double x = xmin; x < xmax; x += step)
			{
				for (double y = ymin; y < ymax; y += stepy)
				{
					double state[] = { x, y };
					double vec[] = { ss2.m_model.value(0, state, param), ss2.m_model.value(1, state, param) };
					double len = std::sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
					vec[0] /= len;
					vec[1] /= len;
#if 1
					if (vec[0] < -null_thr)
						vec[0] = -ideal_len;
					else if (vec[0] > null_thr)
						vec[0] = ideal_len;
					else
						vec[0] = 0;

					if (vec[1] < -null_thr)
						vec[1] = -ideal_leny;
					else if (vec[1] > null_thr)
						vec[1] = ideal_leny;
					else
						vec[1] = 0;
#else
					vec[0] *= ideal_len;
					vec[1] *= ideal_len;
#endif
					std::cout << x << " "
						<< y << " "
						<< vec[0] << " "
						<< vec[1] << "\n";
				}
			}
			std::cout << "e\npause mouse" << std::endl;
			return 0;
		}

		if (arg == "-i")
		{
			std::cout << ss2.m_init << std::endl;
		}

		if (arg == "-f")
		{
			std::cout << ss2.m_final << std::endl;
		}

		if (arg == "-r")
		{
			std::size_t amount = 5;
			if (i + 1 < argc)
				amount = boost::lexical_cast<std::size_t>(argv[++i]);
			ss3.refine_thresholds(amount);
		}*/
	}

	
	std::deque<std::pair<std::string, pp_t> > sch_queue;
	sch_queue.push_back(std::make_pair("0", std::move(pp3)));

	for (; !sch_queue.empty(); sch_queue.pop_front())
	{

		schedule_entry const & sch = schedule[sch_queue.front().first];

		std::cout << "# " << sch_queue.front().first << ": " << sch_queue.front().second << std::endl;


		
		sba_t sba(bas[sch_queue.front().first], ss3);
		
		default_pmc_visitor visitor(show_counterexamples, show_base_coloring, verbose);
		
		succ sucak(thread_count);
		
		
		pp_t violating_parameters = nonnaive(sba, sch_queue.front().second, visitor, sucak);
		
		if (verbose)
			std::cout << "reachable vertices: " << visitor.reachable_vertices << std::endl;
		
		pp_t correct_parameters = sch_queue.front().second;
		correct_parameters.set_difference(violating_parameters);
		
		std::cout << sch.succ_message << "true set : " << sch.succ_next << ": " << correct_parameters << std::endl;
		if (!sch.succ_next.empty())
			sch_queue.push_back(std::make_pair(sch.succ_next, std::move(correct_parameters)));
		
		std::cout << sch.fail_message << "refuted set: " << sch.fail_next << ": " << violating_parameters << std::endl;
		if (!sch.fail_next.empty())
			sch_queue.push_back(std::make_pair(sch.fail_next, std::move(violating_parameters)));

		std::cout << std::endl;
	}
	
}

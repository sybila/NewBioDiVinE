IS OUT OF PROJECT

#ifndef BIO_PARSER_HPP
#define BIO_PARSER_HPP

// FIXME: iostream is used for std::cerr output,
// which should be removed and replace with a custom exception.
#include <iostream>

#include <istream>
#include <string>
#include <map>
#include <set>

#include "state_space.hpp"

#include "paramset.hpp"
#include "buchi_automaton.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#include "bio_eq_parser.hpp"

struct schedule_entry
{
	std::string succ_next;
	std::string succ_message;
	std::string fail_next;
	std::string fail_message;
};

template <typename T, typename Tag>
void parse_bio(std::istream & fin,
	state_space<T> & ss,
	paramset<T, Tag> & pp,
	std::map<std::string, buchi_automaton<typename state_space<T>::proposition> > & bas,
	std::vector<std::string> & varnames,
	std::map<std::string, schedule_entry> & schedule)
{
	typedef typename state_space<T>::proposition prop_t;
	typedef buchi_automaton<prop_t> ba_t;

	model<T> & mm = ss.m_model;

	std::vector<std::set<T> > thresholds;

	std::map<std::string, std::size_t> vars;
	std::map<std::string, std::size_t> params;
	std::map<std::string, std::string> macros;

	std::vector<std::pair<T, T> > param_ranges;

	paramset<T> real_init, real_fair;

	bio_eq_parser<T> eq_parser;

	/* samotne parsovani - odstranit */
	std::string line;
	for (int lineno = 1; std::getline(fin, line); ++lineno)  //browsing of lines
	{
		boost::trim(line);

		// Identify the words and replace them with macros
		{
			std::string new_line;

			auto start = line.begin(); //iterator
			bool in_word = false;
			for (auto ci = line.begin(); ci != line.end(); ++ci) //browsing of chars
			{
				bool alnum = ('A' <= *ci && *ci <= 'Z') || ('a' <= *ci && *ci <= 'z') || ('0' <= *ci && *ci <= '9') || *ci == '_'; //chars control

				if (!in_word && alnum)  //read a word
				{
					in_word = true;
					new_line.append(start, ci);
					start = ci;
				}

				if (in_word && !alnum)  //special char
				{
					in_word = false;
					std::string sub(start, ci);

					auto macro_ci = macros.find(sub);
					if (macro_ci != macros.end())  
						new_line.append(macro_ci->second);
					else  //if start-ci is a macro
						new_line.append(start, ci);
					start = ci;
				}
			}

			new_line.append(start, line.end());
			line = std::move(new_line);
		}

		std::vector<std::string> cmd_args;
		boost::split(cmd_args, line, boost::is_any_of(":"));
		if (cmd_args.empty())
			continue;

		if (cmd_args.size() == 1)
		{
			// Macro definition?
			boost::split(cmd_args, cmd_args[0], boost::is_any_of("="));
			if (cmd_args.size() == 2)
			{
				boost::trim(cmd_args[0]);
				boost::trim(cmd_args[1]);
				macros[cmd_args[0]] = cmd_args[1];
			}
			continue;
		}

		try
		{
			std::string const & cmd = cmd_args[0];
			if (cmd == "VARS")
			{
				std::vector<std::string> var_names;  //variables
				boost::split(var_names, cmd_args[1], boost::is_any_of(","));

				for (std::size_t i = 0; i < var_names.size(); ++i)
					vars[var_names[i]] = i;

				mm.dims(vars.size());
				thresholds.resize(vars.size());
			}
			else if (cmd == "PARAMS")
			{
				std::vector<std::string> var_names;
				boost::split(var_names, cmd_args[1], boost::is_any_of(";")); // split parameters

				for (std::size_t i = 0; i < var_names.size(); ++i)
				{
					std::vector<std::string> args;
					boost::split(args, var_names[i], boost::is_any_of(",")); // split values of interval

					params[args[0]] = param_ranges.size();
					param_ranges.push_back(std::make_pair(boost::lexical_cast<T>(args[1]), boost::lexical_cast<T>(args[2])));
				}
			}
			else if (cmd == "THRES")
			{
				std::size_t dim = vars[cmd_args[1]];

				std::vector<std::string> values;
				boost::split(values, cmd_args[2], boost::is_any_of(","));

				for (std::size_t i = 0; i < values.size(); ++i)
					thresholds[dim].insert(boost::lexical_cast<T>(values[i]));
			}
			else if (cmd == "INIT")
			{
				std::vector<std::pair<T, T> > region;

				std::vector<std::string> dims;
				boost::split(dims, cmd_args[1], boost::is_any_of(";"));
				for (std::size_t i = 0; i < dims.size(); ++i)
				{
					std::vector<std::string> args;
					boost::split(args, dims[i], boost::is_any_of(","));

					T min = boost::lexical_cast<T>(args[0]);
					T max = boost::lexical_cast<T>(args[1]);
					region.push_back(std::make_pair(min, max));

					thresholds[i].insert(min);
					thresholds[i].insert(max);
				}

				real_init.add_region(region.begin(), region.end(), boost::none);
			}
			else if (cmd == "FAIR")
			{
				std::vector<std::pair<T, T> > region;

				std::vector<std::string> dims;
				boost::split(dims, cmd_args[1], boost::is_any_of(";"));
				for (std::size_t i = 0; i < dims.size(); ++i)
				{
					std::vector<std::string> args;
					boost::split(args, dims[i], boost::is_any_of(","));

					T min = boost::lexical_cast<T>(args[0]);
					T max = boost::lexical_cast<T>(args[1]);
					region.push_back(std::make_pair(min, max));

					thresholds[i].insert(min);
					thresholds[i].insert(max);
				}

				real_fair.add_region(region.begin(), region.end(), boost::none);
			}
			else if (cmd == "SCH")
			{
				schedule_entry & sch = schedule[cmd_args[1]];
				sch.succ_next = cmd_args[2];
				sch.succ_message = cmd_args[3];
				sch.fail_next = cmd_args[4];
				sch.fail_message = cmd_args[5];
			}
			else if (cmd == "BA")
			{
				std::string name = "0";

				std::size_t arg_base = 1;
				if (cmd_args.size() == 4)
				{
					name = cmd_args[1];
					arg_base = 2;
				}

				typename ba_t::state state;
				state.final = cmd_args[arg_base] == "F";

				std::vector<std::string> edges;
				boost::split(edges, cmd_args[arg_base + 1], boost::is_any_of(";"));
				for (std::size_t i = 0; i < edges.size(); ++i)
				{
					std::vector<std::string> edge_args;
					boost::split(edge_args, edges[i], boost::is_any_of(","));

					std::vector<std::string> clauses;
					boost::split(clauses, edge_args[0], boost::is_any_of("&"));

					prop_t prop;
					for (std::size_t j = 0; j < clauses.size(); ++j)
					{
						if (clauses[j].empty())
							continue;

						std::vector<std::string> lits;
						boost::split(lits, clauses[j], boost::is_any_of("|"));

						std::vector<typename prop_t::literal_type> clause;
						for (std::size_t k = 0; k < lits.size(); ++k)
						{
							std::vector<std::string> lit_args;
							boost::split(lit_args, lits[k], boost::is_any_of("<"));

							typename prop_t::literal_type lit;
							lit.negate = (lit_args[0][0] == '!');
							if (lit.negate)
								lit_args[0].erase(0, 1);
							lit.dim = vars[lit_args[0]];
							lit.value = boost::lexical_cast<T>(lit_args[1]);
							thresholds[lit.dim].insert(lit.value);
							clause.push_back(lit);
						}

						prop.m_clauses.push_back(clause);
					}

					state.next.push_back(std::make_pair(prop, boost::lexical_cast<std::size_t>(edge_args[1])));
				}

				bas[name].m_states.push_back(state);
			}
			else if (cmd == "EQ")
			{
				std::vector<std::string> temp;
				boost::split(temp, cmd_args[1], boost::is_any_of("=")); // two sides of reaction
				if (temp.size() != 2)
					throw std::runtime_error("syntax error");
				boost::trim(temp[0]);  // left side

				expression<T> expr = eq_parser.parse(temp[1], vars, params);
				mm.set(vars[temp[0].substr(1)], expr);

				// Add thresholds at the edges of the ramps.
				for (std::size_t i = 0; i < expr.size(); ++i)
				{
					for (std::size_t j = 0; j < expr[i].m_ramps.size(); ++j)
					{
						thresholds[expr[i].m_ramps[j].dim].insert(expr[i].m_ramps[j].min);
						thresholds[expr[i].m_ramps[j].dim].insert(expr[i].m_ramps[j].max);
					}
				}
			}
			else if (cmd == "MACRO")
			{
				macros[cmd_args[1]] = cmd_args[2];
			}

		}
		catch (...)
		{
			std::cerr << "error in .bio file on line " << lineno << std::endl;
			std::cerr << line << std::endl;
			throw;
		}
	}

	ss.m_thresholds.resize(thresholds.size());
	for (std::size_t i = 0; i < thresholds.size(); ++i)
		ss.m_thresholds[i].assign(thresholds[i].begin(), thresholds[i].end());

	pp.add_region(param_ranges.begin(), param_ranges.end());

	for (std::size_t i = 0; i < real_init.regions().size(); ++i)
	{
		std::vector<std::pair<std::size_t, std::size_t> > disc_region;
		for (std::size_t j = 0; j < real_init.regions()[i].box.size(); ++j)
			disc_region.push_back(std::make_pair(
				std::lower_bound(ss.m_thresholds[j].begin(), ss.m_thresholds[j].end(), real_init.regions()[i].box[j].first) - ss.m_thresholds[j].begin(),
				std::lower_bound(ss.m_thresholds[j].begin(), ss.m_thresholds[j].end(), real_init.regions()[i].box[j].second) - ss.m_thresholds[j].begin()
			));
		ss.m_init.add_region(disc_region.begin(), disc_region.end(), boost::none);
	}

	for (std::size_t i = 0; i < real_fair.regions().size(); ++i)
	{
		std::vector<std::pair<std::size_t, std::size_t> > disc_region;
		for (std::size_t j = 0; j < real_fair.regions()[i].box.size(); ++j)
			disc_region.push_back(std::make_pair(
				std::lower_bound(ss.m_thresholds[j].begin(), ss.m_thresholds[j].end(), real_fair.regions()[i].box[j].first) - ss.m_thresholds[j].begin(),
				std::lower_bound(ss.m_thresholds[j].begin(), ss.m_thresholds[j].end(), real_fair.regions()[i].box[j].second) - ss.m_thresholds[j].begin()
			));
		ss.m_final.add_region(disc_region.begin(), disc_region.end(), boost::none);
	}

	varnames.resize(vars.size());
	for (auto it = vars.begin(); it != vars.end(); ++it)
		varnames[it->second] = std::move(it->first);

	if (ss.m_init.empty())
	{
		std::vector<std::pair<std::size_t, std::size_t> > region;
		for (std::size_t i = 0; i < ss.m_thresholds.size(); ++i)
			region.push_back(std::make_pair(0, ss.m_thresholds[i].size() - 1));
		ss.m_init.add_region(region.begin(), region.end(), boost::none);
	}

	if (ss.m_final.empty())
	{
		std::vector<std::pair<std::size_t, std::size_t> > region;
		for (std::size_t i = 0; i < ss.m_thresholds.size(); ++i)
			region.push_back(std::make_pair(0, ss.m_thresholds[i].size() - 1));
		ss.m_final.add_region(region.begin(), region.end(), boost::none);
	}
}


#endif

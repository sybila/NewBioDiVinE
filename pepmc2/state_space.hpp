#ifndef STATE_SPACE_HPP
#define STATE_SPACE_HPP

#include <map>
#include <set>
#include <queue>


#include "model.hpp"
#include "paramset.hpp"
#include "buchi_automaton.hpp"

#include "../src/system/bio/data_model/Model.h"

#include <boost/optional.hpp>
#include <boost/range.hpp>
#include <boost/functional/hash.hpp>
#include <vector>

template <typename T>
class state_space
{
public:
	typedef T value_type;
	typedef model<T> model_type;

	state_space()
	{
	}

	template <typename ThresholdIter>
	state_space(model_type const & model, ThresholdIter thresholds)
		: m_model(model)
	{
		for (std::size_t i = 0; i < m_model.dims(); ++i, ++thresholds)
		{
			m_thresholds.push_back(std::vector<value_type>(boost::begin(*thresholds), boost::end(*thresholds)));
			std::sort(m_thresholds.back().begin(), m_thresholds.back().end());
		}
	}

	// TODO: make this private
	model_type m_model;
	std::vector<std::vector<value_type> > m_thresholds;
	paramset<std::size_t> m_init;
	paramset<std::size_t> m_final;

	struct vertex_descriptor
	{
		std::vector<std::size_t> coords;

		vertex_descriptor()
		{
		}

		explicit vertex_descriptor(std::size_t i)
			: coords(i)
		{
		}

		bool empty() const
		{
			return coords.empty();
		}

		friend std::ostream & operator<<(std::ostream & out, vertex_descriptor const & v)
		{
			if (v.coords.empty())
				out << "(";
			else
				out << "(" << v.coords[0];
			for (std::size_t i = 1; i < v.coords.size(); ++i)
				out << ", " << v.coords[i];
			return out << ")";
		}

		friend bool operator==(vertex_descriptor const & lhs, vertex_descriptor const & rhs)
		{
			return lhs.coords == rhs.coords;
		}

		friend bool operator<(vertex_descriptor const & lhs, vertex_descriptor const & rhs)
		{
			return lhs.coords < rhs.coords;
		}

		friend std::size_t hash_value(vertex_descriptor const & v)
		{
			return boost::hash_value(v.coords);
		}
	};

	template <typename State>
	class real_state
	{
	public:
		real_state(state_space const & ss, State state)
			: m_ss(ss), m_state(state)
		{
		}

		value_type operator[](std::size_t i) const
		{
			return m_ss.m_thresholds[i][m_state[i]];
		}

	private:
		state_space const & m_ss;
		State m_state;
	};

	template <typename State, typename Param>
	value_type value(std::size_t dim, State const & state, Param const & param) const
	{
		return m_model.value(dim, real_state<State const &>(*this, state), param);
	}

	bool final(vertex_descriptor const & v) const
	{
		for (std::size_t i = 0; i < m_final.regions().size(); ++i)
		{
			bool res = true;

			std::vector<std::pair<std::size_t, std::size_t> > const & box = m_final.regions()[i].box;
			for (std::size_t j = 0; res && j < box.size(); ++j)
			{
				if (box[j].first > v.coords[j] || v.coords[j] >= box[j].second)
					res = false;
			}

			if (res)
				return true;
		}
		return false;
	}

	class init_enumerator
	{
	public:
		init_enumerator(state_space const & g)
			: m_g(g), m_region_index(0), m_value(g.m_model.dims())
		{
			this->reset_region();
		}

		bool valid() const
		{
			return m_region_index < m_g.m_init.regions().size();
		}

		void next()
		{
			bool carry = true;
			for (std::size_t i = 0; carry && i < m_g.m_init.regions()[m_region_index].box.size(); ++i)
			{
				++m_value.coords[i];
				carry = m_value.coords[i] >= m_g.m_init.regions()[m_region_index].box[i].second;
				if (carry)
					m_value.coords[i] = m_g.m_init.regions()[m_region_index].box[i].first;
			}

			if (carry)
			{
				++m_region_index;
				this->reset_region();
			}
		}

		vertex_descriptor get() const
		{
			return m_value;
		}

	private:
		void reset_region()
		{
			if (m_region_index < m_g.m_init.regions().size())
			{
				for (std::size_t i = 0; i < m_g.m_init.regions()[m_region_index].box.size(); ++i)
					m_value.coords[i] = m_g.m_init.regions()[m_region_index].box[i].first;
			}
		}

		state_space const & m_g;
		std::size_t m_region_index;
		vertex_descriptor m_value;
	};

	class out_edge_enumerator
	{
	public:
		out_edge_enumerator(state_space const & g, vertex_descriptor const & source)
			: m_g(&g), m_source(source), m_dim(0), m_up(false)
		{
			this->find_next();
		}

		void reset()
		{
			m_dim = 0;
			m_up = false;
			this->find_next();
		}

		vertex_descriptor const & source() const
		{
			return m_source;
		}

		vertex_descriptor target() const
		{
			vertex_descriptor res = m_source;
			if (m_up)
				++res.coords[m_dim];
			else
				--res.coords[m_dim];
			return res;
		}

		template <typename Tag>
		void paramset_intersect(paramset<T, Tag> & p) const
		{
			BOOST_ASSERT(m_dim < m_source.coords.size());

			paramset<T, Tag> removed = std::move(p);
			p.clear();

			std::vector<bool> shifts(m_g->m_model.dims());
			shifts[m_dim] = m_up;

			std::vector<value_type> pt(m_g->m_model.dims());

			bool carry = false;
			while (!carry)
			{
				for (std::size_t i = 0; i < pt.size(); ++i)
					pt[i] = m_g->m_thresholds[i][m_source.coords[i] + shifts[i]];

				m_g->m_model.paramset_transfer(removed, p, pt, m_dim, m_up);
				if (removed.empty())
					break;

				carry = true;
				for (std::size_t i = 0; carry && i < shifts.size(); ++i)
				{
					if (i == m_dim)
						continue;

					carry = shifts[i];
					shifts[i] = true;
				}
			}
		}

		bool valid() const
		{
			return m_dim < m_source.coords.size();
		}

		void next()
		{
			BOOST_ASSERT(m_dim < m_source.coords.size());

			if (m_up)
				++m_dim;
			m_up = !m_up;
			this->find_next();
		}

		state_space const * m_g;
		vertex_descriptor m_source;
		std::size_t m_dim;
		bool m_up;

	private:
		void find_next()
		{
			while (
				m_dim < m_source.coords.size()
				&& ((!m_up && m_source.coords[m_dim] == 0)
					|| (m_up && m_source.coords[m_dim] + 2 == m_g->m_thresholds[m_dim].size())
					)
				)
			{
				if (m_up)
					++m_dim;
				m_up = !m_up;
			}
		}
	};

	std::size_t dims() const
	{
		return m_model.dims();
	}

	template <typename Paramset>
	void remove_nontransient(Paramset & p_down, Paramset & p_up, vertex_descriptor const & v, std::size_t dim) const
	{
		std::vector<value_type> pt(m_model.dims());
		std::vector<bool> shifts(m_model.dims());

		bool carry = false;
		while (!carry)
		{
			for (std::size_t i = 0; i < pt.size(); ++i)
				pt[i] = m_thresholds[i][v.coords[i] + shifts[i]];

			m_model.fix_paramsets(p_down, p_up, pt, dim);

			if (p_down.empty() && p_up.empty())
				break;

			carry = true;
			for (std::size_t i = 0; carry && i < shifts.size(); ++i)
			{
				carry = shifts[i];
				shifts[i] = true;
			}
		}
	}

	template <typename Paramset>
	void self_loop(vertex_descriptor const & v, Paramset & p) const
	{
		for (std::size_t dim = 0; dim < m_model.dims(); ++dim)
		{
			Paramset p_down = p;
			Paramset p_up = p;
			this->remove_nontransient(p_down, p_up, v, dim);
			p.set_difference(p_down);
			p.set_difference(p_up);
		}
	}

	struct literal_proposition
	{
		std::size_t dim;
		value_type value;
		bool negate;
	};

	// conjunctive normal form
	struct proposition
	{
		typedef literal_proposition literal_type;
		std::vector<std::vector<literal_proposition> > m_clauses;
	};

	bool valid(proposition const & prop, vertex_descriptor const & v) const
	{
		for (std::size_t i = 0; i < prop.m_clauses.size(); ++i)
		{
			bool valid = false;
			for (std::size_t j = 0; !valid && j < prop.m_clauses[i].size(); ++j)
			{
				if (prop.m_clauses[i][j].negate)
				{
					valid = m_thresholds[prop.m_clauses[i][j].dim][v.coords[prop.m_clauses[i][j].dim]] >= prop.m_clauses[i][j].value;
				}
				else
				{
					valid = m_thresholds[prop.m_clauses[i][j].dim][v.coords[prop.m_clauses[i][j].dim]] < prop.m_clauses[i][j].value;
				}
			}

			if (!valid)
				return false;
		}
		return true;
	}

	void refine_thresholds(std::size_t amount)
	{
		std::vector<std::vector<value_type> > new_thresholds;
		for (std::size_t i = 0; i < m_thresholds.size(); ++i)
		{
			std::vector<value_type> const & thr = m_thresholds[i];
			std::vector<value_type> v(1, thr[0]);

			for (std::size_t j = 1; j < thr.size(); ++j)
			{
				for (std::size_t k = 0; k < amount + 1; ++k)
					v.push_back(thr[j-1] + (thr[j] - thr[j-1])/(amount+1)*(k+1));
			}

			new_thresholds.push_back(v);
		}

		m_thresholds = new_thresholds;

		for (std::size_t i = 0; i < m_init.regions().size(); ++i)
		{
			for (std::size_t j = 0; j < m_init.regions()[i].box.size(); ++j)
			{
				m_init.regions()[i].box[j].first *= amount + 1;
				m_init.regions()[i].box[j].second *= amount + 1;
			}
		}

		for (std::size_t i = 0; i < m_final.regions().size(); ++i)
		{
			for (std::size_t j = 0; j < m_final.regions()[i].box.size(); ++j)
			{
				m_final.regions()[i].box[j].first *= amount + 1;
				m_final.regions()[i].box[j].second *= amount + 1;
			}
		}
	}

	// Should remove leading and terminating white space
	void  trim(std::string& _l)
	{
		if (!_l.empty())
		{
			_l.erase(_l.find_last_not_of(" \t\n") + 1);
			if (!_l.empty())
			{
				size_t p = _l.find_first_not_of(" \t\n");
				if (p != std::string::npos)
					_l.erase(0, p);
			}
		}
	}



	// Splits a string according to a separator. The result is stored in a queue of strings.
	std::queue<std::string>  split(std::string _separator, std::string _what)
	{
		std::queue<std::string> retval;
		while (!_what.empty())
		{
			size_t pos = _what.find(_separator);
			std::string s;
			if (pos == std::string::npos)
			{
				s = _what;
				_what.clear();
			}
			else
			{
				s = _what.substr(0, pos - (_separator.size() - 1));
				_what.erase(0, pos + _separator.size());
			}
			trim(s);
			retval.push(s);
			//std::cout <<"split:"<<s<<" _what="<<_what<<std::endl;
		}
		return retval;
	}

	// Should return true if the string starts with '#' or "//"
	bool is_comment(std::string _l)
	{
		trim(_l);
		if ((_l[0] == '#') ||
			(_l[0] == '/' && _l[1] == '/')
			)
			return true;
		return false;
	}

	struct parsing_state {
		string name;
		std::size_t id;
		bool final;

		struct parsing_transition {
			string name2;
			std::size_t id2;

			struct parsing_clause {
				size_t dim;
				double value;
				bool negate;
			};

			std::vector<parsing_clause> parsing_clauses;

		};

		std::vector<parsing_transition> parsing_transitions;


	};

	template <typename Temp>
	void initialize(std::istream &in, Model<Temp> &storage, std::vector<std::pair<Temp, Temp> > param_ranges, std::vector<std::string> & varnames,
		std::map<std::string, buchi_automaton<typename state_space<Temp>::proposition> > & bas)
	{
		// Definitions
		// Variables
		std::vector<std::string> var_names(storage.getVariables());
		std::map<std::string, std::size_t> vars;
		// Parameters
		std::map<std::string, std::size_t> params;
		
		// Initializations
		paramset<T> real_init, real_fair;
		// Thresholds
		std::vector<std::set<T> > thresholds;


		// TODO
		std::map<std::string, std::string> macros;

		

		// Initialization
		// Variables
		for (std::size_t i = 0; i < var_names.size(); ++i)
			vars[var_names[i]] = i;

		m_model.dims(vars.size());
		thresholds.resize(vars.size());		

		// Parameters
		for (std::size_t i = 0; i < storage.getParamNames().size(); ++i)
			params[storage.getParamName(i)] = i;

		// Initializations
		std::vector<std::pair<T, T> > region(storage.getInitsValues());
		real_init.add_region(region.begin(), region.end(), boost::none);

		// Thresholds
		for (std::size_t i = 0; i < thresholds.size(); ++i)
			for (std::size_t j = 0; j < storage.getThresholdsForVariable(i).size(); ++j)
				thresholds[i].insert(storage.getThresholdsForVariable(i).at(j));
		
		int i = 0;
		for (auto it = region.begin(); it != region.end(); it++)
		{
			thresholds[i].insert((*it).first);
			thresholds[i].insert((*it).second);
			i++;
		}
				


		// Equations
		expression<T> equations;
		

		const std::vector<std::pair<std::size_t, typename std::vector<Summember<T> > > > eeq = storage.getEquations();
		
		
		for (auto it = eeq.begin(); it != eeq.end(); it++)
		{		
			for (auto it_sum =  (*it).second.begin(); it_sum != (*it).second.end(); it_sum++)
			{
				summember<T> sum;

				std::vector<std::size_t> sum_var = (*it_sum).GetVars();
				for (auto it_sum_var = sum_var.begin(); it_sum_var != sum_var.end(); it_sum_var++)
				{
					sum.add_var((*it_sum_var)-1);
				}

				std::vector<typename Summember<T>::ramp> sum_ramp = (*it_sum).GetRamps();
				for (auto it_sum_ramp = sum_ramp.begin(); it_sum_ramp != sum_ramp.end(); it_sum_ramp++)
				{
					typename Summember<T>::ramp r = (*it_sum_ramp);
					
					///
					thresholds[r.dim-1].insert(r.min);
					thresholds[r.dim-1].insert(r.max);

					sum.add_ramp(r.dim-1, r.min, r.max, r.min_value, r.max_value, r.negative);
				}


				sum.add_param((*it_sum).GetParam()-1);
				sum.add_constant((*it_sum).GetConstant());
				
				m_model.add_summember((*it).first, sum);
			}

		}


		/////////////////////////////////////////////////////////////
		
		
		std::string modelline;
		std::string line;
		for (int lineno = 1; std::getline(in, line); ++lineno)  //browsing of lines
		{
			if (is_comment(line)) continue;
			trim(line);			
			modelline += line;
		}
		
			size_t start, end;

			start = modelline.find("process");
			end = modelline.find("system", start);
			
			if (start == std::string::npos || end == std::string::npos) {
				std::cout << "Bio file error in LTL" << std::endl;
				exit(1);
			}


			std::string ltl_part = modelline.substr(start, end - start);
			trim(ltl_part);

			
			const bool dbg = true; //TODO
			if (dbg) std::cout << "Parsing token: ==" << ltl_part << "==" << std::endl;
			size_t pos;
			std::queue<std::string> fields;
			std::queue<std::string> fields1;
			


			

			







































			

			std::vector<parsing_state> parsing_states;
			if ((pos = ltl_part.find("process")) != std::string::npos)
			{
				ltl_part.erase(0, 7);
				trim(ltl_part);
				pos = ltl_part.find_first_of('{');
				ltl_part.erase(0, pos + 1);
				ltl_part.erase(ltl_part.size() - 1, 1);
				trim(ltl_part);


				// First we split the string representing the property process 
				// into state-definition part and transition-definition part.
				pos = ltl_part.find("trans");
				std::string procstates = ltl_part.substr(0, pos);
				ltl_part.erase(0, procstates.size() + 5);

				// now we split state definition according to ';'
				fields = split(";", procstates);
				
						
				std::vector<std::string> clauses;
				std::vector<std::string> states;

				// now we iterate, and identify individual parts
				while (!fields.empty())
				{
					int mode = 0;
					if (dbg) std::cerr << fields.front() << std::endl;
					if (fields.front().find("state") == 0)
					{
						fields.front().erase(0, 6);
						mode = 1;
					}

					if (fields.front().find("init") == 0)
					{
						fields.front().erase(0, 5);
						mode = 2;
					}

					if (fields.front().find("accept") == 0)
					{
						fields.front().erase(0, 7);
						mode = 3;
					}

					// now having one part, we split accordin to ',' to get individual states
					size_t init_id;
					parsing_state ps;
					string n = "";
					size_t succ_id = 0;
					fields1 = split(",", fields.front());
					while (!fields1.empty())
					{
						switch (mode) {
						case 1:					
							ps.name = fields1.front();	
							ps.id = succ_id;
							ps.final = false;
							parsing_states.push_back(ps);

							succ_id++;
							break;
						case 2:							
							for (auto it = parsing_states.begin(); it != parsing_states.end(); it++)
							{
								n = (*it).name;
								if (0 == n.compare(fields1.front())) {
									init_id = (*it).id;
									(*it).id = 0;
									break;
								}
							}

							for (auto it = parsing_states.begin(); it != parsing_states.end(); it++)
							{
								if ((*it).id == 0) {
									
									(*it).id = init_id;
									break;
								}
							}

							break;
						case 3:
							for (auto it = parsing_states.begin(); it != parsing_states.end(); it++)
							{
								n = (*it).name;
								if (0 == n.compare(fields1.front())) {									
									(*it).final = true;
									break;
								}
							}

							break;
						}
						fields1.pop();
					}
					fields.pop();
				}


				//Here we parse the transition-definition part of the string representing property process.

				ltl_part.erase(ltl_part.size() - 1, 1); // erase terminating ';'				
				fields = split(",", ltl_part);
				
				while (!fields.empty())
				{
					fields.front().erase(fields.front().size() - 1, 1); // erase terminating '}'
					fields1 = split("{", fields.front());
					pos = fields1.front().find("->");
					std::string sname1 = fields1.front().substr(0, pos);
					trim(sname1);
					std::string sname2 = fields1.front().substr(pos + 2, fields1.front().size() - pos - 2);
					trim(sname2);
					fields1.pop();
					string n = "";

					if (dbg) std::cerr << "prop trans:" << sname1 << "->" << sname2 << std::endl;
					if (fields1.empty()) // empty transition ===> true
					{
						for (auto it = parsing_states.begin(); it != parsing_states.end(); it++)
						{
							n = (*it).name;
							if (0 == n.compare(sname1)) {
								typename parsing_state::parsing_transition pt;
								pt.name2 = sname2;
								string nn = "";
								for (auto parsing_states_iterator = parsing_states.begin(); parsing_states_iterator != parsing_states.end(); parsing_states_iterator++)
								{
									nn = (*parsing_states_iterator).name;
									if (0 == nn.compare(sname2)) {
										pt.id2 = (*parsing_states_iterator).id;
									}

								}

								((*it).parsing_transitions).push_back(pt);
								break;
							}
						}

						if (dbg) std::cerr << " + true" << std::endl;
					}
					else
					{
						fields1.front().erase(fields1.front().size() - 1, 1); // erase terminating ';'
						fields1.front().erase(0, 6); // erase leading 'guard '
						std::string aux = fields1.front();
						std::string literal;
						fields1.pop();
						fields1 = split("&&", aux);

						while (!fields1.empty())
						{
							
							//..... parsing Atomic Proposition ....
							if (fields1.front().find("!") != std::string::npos)
							{
								std::cerr << "Atomic proposition contains !, which is not allowed." << std::endl;
								exit(1);
							}

							if (fields1.front().find("==") != std::string::npos)
							{
								std::cerr << "Atomic proposition contains ==, which is not allowed." << std::endl;
								exit(1);
							}

							bool negated = false;
							//affine_ap_t AP;
							std::string APstr = fields1.front();

							//Remove possible ()
							if (APstr.find("(") != std::string::npos)
							{
								if (APstr.find("(") != 0 || APstr.find(")") != APstr.size() - 1)
								{
									std::cout << "cannot parse atomic proposition " + APstr;
								}
								APstr.erase(0, 1);
								APstr.erase(APstr.size() - 1, 1);
							}
							trim(APstr);

							//Test for true statement
							if (APstr.find("true") != std::string::npos ||
								APstr.find("True") != std::string::npos ||
								APstr.find("TRUE") != std::string::npos
								)
							{
								
								for (auto it = parsing_states.begin(); it != parsing_states.end(); it++)
								{
									n = (*it).name;
									if (0 == n.compare(sname1)) {
										typename parsing_state::parsing_transition pt;
										pt.name2 = sname2;

										string nn = "";
										for (auto parsing_states_iterator = parsing_states.begin(); parsing_states_iterator != parsing_states.end(); parsing_states_iterator++)
										{
											nn = (*parsing_states_iterator).name;
											if (0 == nn.compare(sname2)) {
												pt.id2 = (*parsing_states_iterator).id;
											}

										}


										((*it).parsing_transitions).push_back(pt);
										break;
									}
								}

								fields1.pop();
								continue;
							}

							if (APstr.find("not") == 0)
							{
								APstr.erase(0, 4);
								negated = true;
							}
							std::string text_repr = APstr;
							pos = APstr.find_first_of("!>=<");
							literal = APstr.substr(0, pos);
							trim(literal);
							//size_t var = get_varid(aux);
							
							APstr.erase(0, pos);
							pos = APstr.find_first_not_of("!>=<");
							aux = APstr.substr(pos, APstr.size() - pos);
							trim(aux);
							APstr.erase(pos, APstr.size() - pos);
							trim(APstr);

							
							for (auto it = parsing_states.begin(); it != parsing_states.end(); it++)
							{
								n = (*it).name;
								if (0 == n.compare(sname1)) {
									typename parsing_state::parsing_transition pt;
									pt.name2 = sname2;

									string nn = "";
									for (auto parsing_states_iterator = parsing_states.begin(); parsing_states_iterator != parsing_states.end(); parsing_states_iterator++)
									{
										nn = (*parsing_states_iterator).name;
										if (0 == nn.compare(sname2)) {
											pt.id2 = (*parsing_states_iterator).id;
										}

									}
									
									typename parsing_state::parsing_transition::parsing_clause pc;
									pc.dim = vars[literal];
									pc.value = atof(aux.c_str());


									if (APstr.size() == 2 && std::string::npos != APstr.find(">="))
									{
										negated = !negated;
									}

									if (APstr.size() == 1 && std::string::npos != APstr.find(">"))
									{
										negated = !negated;
									}

									pc.negate = negated;
									
									
									pt.parsing_clauses.push_back(pc);
									((*it).parsing_transitions).push_back(pt);
									break;
								}
							}


							fields1.pop();
							
						}
						
					}

					fields.pop();
					
				}
				

				/* //TODO: Biodivine thing
				set_property_process(prop);
				}
				// ---- system  ----
				if ((pos = _token.find("system")) != std::string::npos)
				{
				if ((pos = _token.find("async")) != std::string::npos)
				{
				has_system_keyword = true;
				}
				}
			*/	

			}
			

			

		typedef typename state_space<T>::proposition prop_t;
		typedef buchi_automaton<prop_t> ba_t;
		typename ba_t::state state;
		for (auto it = parsing_states.begin(); it != parsing_states.end(); it++)
		{

			parsing_state ps = *it;
			std::string name = "0";			

			
			state.final = ps.final;
			

			for (auto it2 = (ps.parsing_transitions).begin(); it2 != (ps.parsing_transitions).end(); it2++)
			{
				typename parsing_state::parsing_transition pt = *it2;
				prop_t prop;

				for (auto it3 = (pt.parsing_clauses).begin(); it3 != (pt.parsing_clauses).end(); it3++)
				{
					typename parsing_state::parsing_transition::parsing_clause pc;
					pc = *it3;

					//if (clauses[j].empty())
						//continue;
					

					std::vector<typename prop_t::literal_type> clause;

	
						typename prop_t::literal_type lit;
						lit.negate = pc.negate;

						lit.dim = pc.dim;
						lit.value = pc.value;
						thresholds[lit.dim].insert(lit.value);
						clause.push_back(lit);
					

					prop.m_clauses.push_back(clause);
					
				}

				state.next.push_back(std::make_pair(prop, pt.id2));
				
			}
		
			
			bas[name].m_states.push_back(state);

		}




		/**********************************************************************************************************************/
		
		m_thresholds.resize(thresholds.size());
		for (std::size_t i = 0; i < thresholds.size(); ++i)
			m_thresholds[i].assign(thresholds[i].begin(), thresholds[i].end());

		

		for (std::size_t i = 0; i < real_init.regions().size(); ++i)
		{
			std::vector<std::pair<std::size_t, std::size_t> > disc_region;
			for (std::size_t j = 0; j < real_init.regions()[i].box.size(); ++j)
				disc_region.push_back(std::make_pair(
				std::lower_bound(m_thresholds[j].begin(), m_thresholds[j].end(), real_init.regions()[i].box[j].first) - m_thresholds[j].begin(),
				std::lower_bound(m_thresholds[j].begin(), m_thresholds[j].end(), real_init.regions()[i].box[j].second) - m_thresholds[j].begin()
				));
			m_init.add_region(disc_region.begin(), disc_region.end(), boost::none);
		}


		for (std::size_t i = 0; i < real_fair.regions().size(); ++i)
		{
			std::vector<std::pair<std::size_t, std::size_t> > disc_region;
			for (std::size_t j = 0; j < real_fair.regions()[i].box.size(); ++j)
				disc_region.push_back(std::make_pair(
				std::lower_bound(m_thresholds[j].begin(), m_thresholds[j].end(), real_fair.regions()[i].box[j].first) - m_thresholds[j].begin(),
				std::lower_bound(m_thresholds[j].begin(), m_thresholds[j].end(), real_fair.regions()[i].box[j].second) - m_thresholds[j].begin()
				));
			m_final.add_region(disc_region.begin(), disc_region.end(), boost::none);
		}



		varnames.resize(vars.size());
		for (auto it = vars.begin(); it != vars.end(); ++it)
			varnames[it->second] = std::move(it->first);

		if (m_init.empty())
		{
			std::vector<std::pair<std::size_t, std::size_t> > region;
			for (std::size_t i = 0; i < m_thresholds.size(); ++i)
				region.push_back(std::make_pair(0, m_thresholds[i].size() - 1));
			m_init.add_region(region.begin(), region.end(), boost::none);
		}

		if (m_final.empty())
		{
			std::vector<std::pair<std::size_t, std::size_t> > region;
			for (std::size_t i = 0; i < m_thresholds.size(); ++i)
				region.push_back(std::make_pair(0, m_thresholds[i].size() - 1));
			m_final.add_region(region.begin(), region.end(), boost::none);
		}

		/************************************************************************************************************/



	}

	friend std::ostream & operator<<(std::ostream & o, state_space const & v)
	{
		return o << v.m_model;
	}
};

#endif

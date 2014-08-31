#ifndef PEPMC_BIO_EQ_PARSER_HPP
#define PEPMC_BIO_EQ_PARSER_HPP

#include <boost/algorithm/string/erase.hpp>

#include "lrparser/rule.hpp"
#include "lrparser/grammar.hpp"
#include "lrparser/table.hpp"
#include "lrparser/terminal_set.hpp"
#include "lrparser/generator.hpp"
#include "lrparser/parser.hpp"

#include "state_space.hpp"

template <typename T>
class bio_eq_parser
{
public:
	typedef T value_type;

	bio_eq_parser()
	{
		grammar<rule_type> g;
		g.add("expr") << "prod";
		g.add("expr", expr_sum) << "expr" << '+' << "prod";
		g.add("expr", expr_diff) << "expr" << '-' << "prod";
		g.add("prod") << "term";
		g.add("prod", expr_prod) << "prod" << '*' << "term";

		g.add("term", negate) << '-' << "term";
		g.add("term") << '(' << "expr" << ')';

		g.add("term", float_to_term) << "float";
		g.add("term", fn_to_term) << "arg" << '(' << "arg_list" << ')';
		g.add("term", name_to_term) << "name";

		g.add("arg_list") << "arg";
//		g.add("arg_list", new_arglist) << "float";
		g.add("arg_list") << "arg_list" << ',' << "arg";
//		g.add("arg_list", append_arglist) << "arg_list" << ',' << "float";

		g.add("arg", make_arg) << "name";
		g.add("arg", make_arg) << "float";

		g.add("name") << "N";
		g.add("name") << "name" << "N";
		g.add("name") << "name" << "M";
		g.add("N") << terminal_set<char>('a', 'z');
		g.add("N") << terminal_set<char>('A', 'Z');
		g.add("N") << '_';

		g.add("float", reduce_float) << "prefloat";
		g.add("prefloat") << "M";
		g.add("prefloat") << "prefloat" << "M";
		g.add("prefloat") << "prefloat" << '.';

		g.add("M") << terminal_set<char>('0', '9');

		generate_lr1_tables_remapped(g.begin(), g.end(), "expr", m_table);
	}

	typedef expression<value_type> expr_t;
	expr_t parse(
		std::string expr,
		std::map<std::string, std::size_t> const & vars,
		std::map<std::string, std::size_t> const & params)
	{
		boost::algorithm::erase_all(expr, " ");

		parse_visitor v(vars, params);
		::parse(m_table, expr.begin(), expr.end(), boost::ref(v));
		return v.m_exprs.back();
	}

private:
	struct parse_visitor;

	typedef void (*action_type)(parse_visitor &, char const *, char const *);
	typedef rule<terminal_set<char>, std::string, action_type> rule_type;

	struct parse_visitor
	{
		parse_visitor(
			std::map<std::string, std::size_t> const & vars,
			std::map<std::string, std::size_t> const & params)
			: m_vars(vars), m_params(params)
		{
		}

		void reduce(action_type action, std::string::const_iterator first, std::string::const_iterator last)
		{
			if (action != 0)
			{
				// FIXME: The following line features an undefined behavior.
				action(*this, &*first, &*first + (last - first));
			}
		}

		std::map<std::string, std::size_t> const & m_vars;
		std::map<std::string, std::size_t> const & m_params;

		std::vector<expr_t> m_exprs;
		std::vector<value_type> m_floats;
		std::vector<std::string> m_arglist;
	};

	static void fn_to_term(parse_visitor & v, char const * first, char const * last)
	{
		if (v.m_arglist.size() == 4) {

			typename summember<value_type>::ramp r;
			r.negative = (v.m_arglist[0] == "rn");
			r.dim = v.m_vars.find(v.m_arglist[1])->second;
			r.min = boost::lexical_cast<value_type>(v.m_arglist[2]);
			r.max = boost::lexical_cast<value_type>(v.m_arglist[3]);
			r.min_value = 0;
			r.max_value = 1;

			summember<value_type> sm;
			sm.m_ramps.push_back(r);

			v.m_exprs.push_back(expression<T>(sm));
			v.m_arglist.clear();
		}

		else if (v.m_arglist.size() == 6) {

			typename summember<value_type>::ramp r;
			r.negative = (v.m_arglist[0] == "rn");
			r.dim = v.m_vars.find(v.m_arglist[1])->second;
			r.min = boost::lexical_cast<value_type>(v.m_arglist[2]);
			r.max = boost::lexical_cast<value_type>(v.m_arglist[3]);
			r.min_value = boost::lexical_cast<value_type>(v.m_arglist[4]);
			r.max_value = boost::lexical_cast<value_type>(v.m_arglist[5]);

			summember<value_type> sm;
			sm.m_ramps.push_back(r);

			v.m_exprs.push_back(expression<T>(sm));
			v.m_arglist.clear();
		} 

		else {
			throw std::runtime_error("wrong number of arguments (expected 3)");
		}
	}

	static void make_arg(parse_visitor & v, char const * first, char const * last)
	{
		v.m_arglist.push_back(std::string(first, last));
	}

	static void reduce_float(parse_visitor & v, char const * first, char const * last)
	{
		v.m_floats.push_back(boost::lexical_cast<value_type>(std::string(first, last)));
	}

	static void negate(parse_visitor & v, char const *, char const *)
	{
		expr_t & e = v.m_exprs.back();
		for (std::size_t i = 0; i < e.size(); ++i)
			e[i].k(-e[i].k());
	}

	static void float_to_term(parse_visitor & v, char const *, char const *)
	{
		v.m_exprs.push_back(expression<T>(summember<value_type>(v.m_floats.back())));
		v.m_floats.pop_back();
	}

	static void name_to_term(parse_visitor & v, char const * first, char const * last)
	{
		summember<value_type> sm;

		std::map<std::string, std::size_t>::const_iterator ci
			= v.m_vars.find(std::string(first, last));
		if (ci != v.m_vars.end())
			sm.add_var(ci->second);
		else
		{
			ci = v.m_params.find(std::string(first, last));
			sm.param(ci->second);
		}
		v.m_exprs.push_back(expression<value_type>(sm));
	}

	static void expr_sum(parse_visitor & v, char const *, char const *)
	{
		expr_t & el = v.m_exprs[v.m_exprs.size() - 2];
		expr_t const & er = v.m_exprs[v.m_exprs.size() - 1];

		for (std::size_t i = 0; i < er.size(); ++i)
			el.add(er[i]);

		v.m_exprs.pop_back();
	}

	static void expr_diff(parse_visitor & v, char const *, char const *)
	{
		expr_t & el = v.m_exprs[v.m_exprs.size() - 2];
		expr_t & er = v.m_exprs[v.m_exprs.size() - 1];

		for (std::size_t i = 0; i < er.size(); ++i)
			er[i].k(-er[i].k());

		for (std::size_t i = 0; i < er.size(); ++i)
			el.add(er[i]);

		v.m_exprs.pop_back();
	}

	static void expr_prod(parse_visitor & v, char const *, char const *)
	{
		expr_t res;

		{
			expr_t const & el = v.m_exprs[v.m_exprs.size() - 2];
			expr_t const & er = v.m_exprs[v.m_exprs.size() - 1];

			for (std::size_t i = 0; i < er.size(); ++i)
			{
				for (std::size_t j = 0; j < el.size(); ++j)
				{
					summember<value_type> sm;

					sm.k(el[j].k()*er[i].k());

					if (el[j].has_param() && er[i].has_param())
						throw std::runtime_error("multiple params per summember");
					if (el[j].has_param())
						sm.param(el[j].param());
					if (er[i].has_param())
						sm.param(er[i].param());

					for (std::size_t k = 0; k < el[j].vars().size(); ++k)
					{
						if (el[j].vars()[k])
							sm.add_var(k);
					}

					for (std::size_t k = 0; k < er[i].vars().size(); ++k)
					{
						if (er[i].vars()[k])
							sm.add_var(k);
					}

					sm.m_ramps = el[j].m_ramps;
					sm.m_ramps.insert(sm.m_ramps.end(), er[i].m_ramps.begin(), er[i].m_ramps.end());

					res.add(sm);
				}
			}
		}

		v.m_exprs.pop_back();
		v.m_exprs.back() = res;
	}

	lr1_table<terminal_set<char>, std::size_t, action_type> m_table;
};

#endif

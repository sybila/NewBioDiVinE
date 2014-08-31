#ifndef MODEL_HPP
#define MODEL_HPP

#include <boost/dynamic_bitset.hpp>
#include <vector>
#include <boost/variant.hpp>
#include "paramset.hpp"

typedef boost::dynamic_bitset<> variable_set;

template <typename T>
class summember
{
public:
	typedef T value_type;

	summember()
		: m_k(1), m_param(0)
	{
	}

	summember(value_type k)
		: m_k(k), m_param(0)
	{
	}

	// TODO: Remove this constructor when the equation parser is finished.
	summember(value_type const & k, std::string const & vars)
		: m_k(k), m_param(0), m_vars(vars.size())
	{
		for (std::size_t i = 0; i < vars.size(); ++i)
			if (vars[i] != '.')
				m_vars.set(i);
	}

	// TODO: Remove this constructor when the equation parser is finished.
	summember(value_type const & k, std::size_t param, std::string const & vars)
		: m_k(k), m_param(param + 1), m_vars(vars.size())
	{
		for (std::size_t i = 0; i < vars.size(); ++i)
			if (vars[i] != '.')
				m_vars.set(i);
	}

	value_type k() const
	{
		return m_k;
	}

	void k(value_type value)
	{
		m_k = value;
	}

	void add_constant(value_type value)
	{
		k(value);
	}

	bool has_param() const
	{
		return m_param != 0;
	}

	std::size_t param() const
	{
		BOOST_ASSERT(m_param != 0);
		return m_param - 1;
	}

	void param(std::size_t value)
	{
		m_param = value + 1;
	}

	void add_param(std::size_t value)
	{
		param(value);
	}

	void clear_param()
	{
		m_param = 0;
	}

	variable_set const & vars() const
	{
		return m_vars;
	}

	void add_var(std::size_t index)
	{
		if (m_vars.size() < index + 1)
			m_vars.resize(index + 1);
		m_vars[index] = true;
	}

	struct ramp
	{
		std::size_t dim;
		value_type min;
		value_type max;
		value_type min_value;
		value_type max_value;
		bool negative;

		value_type value(value_type value) const
		{
			value_type res = (value - min) / (max - min);
			res = min_value + (res * (max_value - min_value));
			if (res < 0)
				res = 0;
			else if (res > 1)
				res = 1;
			return negative? 1 - res: res;
		}
	};

	std::vector<ramp> m_ramps;

	void add_ramp(std::size_t dim, value_type min, value_type max, value_type min_value, value_type max_value, bool negative)
	{
		ramp r;
		r.dim = dim;
		r.min = min;
		r.max = max;
		r.min_value = min_value;
		r.max_value = max_value;
		r.negative = negative;
		
		m_ramps.push_back(r);

	}

	friend std::ostream & operator<<(std::ostream & o, summember const & v)
	{
		o << v.m_k;
		if (v.has_param())
			o << "*p" << v.param();
		for (std::size_t i = 0; i < v.m_vars.size(); ++i)
		{
			if (v.m_vars[i])
				o << "*v" << i;
		}
		for (std::size_t i = 0; i < v.m_ramps.size(); ++i)
			o << (v.m_ramps[i].negative? "*rn(v": "*rp(v")
				<< v.m_ramps[i].dim << ", "
				<< v.m_ramps[i].min << ", "
				<< v.m_ramps[i].max << ", "
				<< v.m_ramps[i].min_value << ", "
				<< v.m_ramps[i].max_value << ")";
		return o;
	}

private:
	value_type m_k;
	std::size_t m_param;
	variable_set m_vars;
};

template <typename T>
struct expression
{
public:
	expression()
	{
	}

	explicit expression(summember<T> const & sm)
		: m_summembers(1, sm)
	{
	}

	std::vector<summember<T> > const & sm() const
	{
		return m_summembers;
	}

	void add(summember<T> const & sm)
	{
		m_summembers.push_back(sm);
	}

	void clear()
	{
		m_summembers.clear();
	}

	bool empty() const
	{
		return m_summembers.empty();
	}

	std::size_t size() const
	{
		return m_summembers.size();
	}

	summember<T> const & operator[](std::size_t i) const
	{
		return m_summembers[i];
	}

	summember<T> & operator[](std::size_t i)
	{
		return m_summembers[i];
	}

	friend std::ostream & operator<<(std::ostream & o, expression const & v)
	{
		if (!v.m_summembers.empty())
			o << v.m_summembers.front();
		for (std::size_t i = 1; i < v.m_summembers.size(); ++i)
			o << " + " << v.m_summembers[i];
		return o;
	}

private:
	std::vector<summember<T> > m_summembers;
};

template <typename T>
class model
{
public:
	typedef T value_type;
	typedef summember<T> summember_type;

	model()
	{
	}

	explicit model(std::size_t dims)
		: m_equations(dims)
	{
	}

	void set(std::size_t var, expression<value_type> const & expression)
	{
		m_equations[var] = expression;
	}

	void add_summember(std::size_t var, summember_type const & s)
	{
		m_equations[var].add(s);
	}

	std::size_t dims() const
	{
		return m_equations.size();
	}

	void dims(std::size_t value)
	{
		m_equations.resize(value);
	}

	// `state` must provide `operator[](std::size_t) -> value_type`.
	template <typename State, typename Param>
	value_type value(std::size_t dim, State const & state, Param const & param) const
	{
		expression<value_type> const & eq = m_equations[dim];
		value_type val = 0;

		for (std::size_t i = 0; i < eq.size(); ++i)
		{
			value_type v = eq[i].k();
			for (std::size_t j = 0; j < eq[i].vars().size(); ++j)
			{
				if (eq[i].vars()[j])
					v *= state[j];
			}
			for (std::size_t j = 0; j < eq[i].m_ramps.size(); ++j)
				v *= eq[i].m_ramps[j].value(state[eq[i].m_ramps[j].dim]);

			if (eq[i].has_param())
				v *= param[eq[i].param()];

			val += v;
		}

		return val;
	}

	// `state` must provide `operator[](std::size_t) -> value_type`.
	//
	// Transfers all `p` from `src` to `dest` such that the derivation at `state` in the direction `dim` is
	//  1. positive, if `up`, or
	//  2. negative, if not `up`.
	template <typename State, typename PT, typename Tag>
	void paramset_transfer(paramset<PT, Tag> & src, paramset<PT, Tag> & dest, State const & state, std::size_t dim, bool up) const
	{
		value_type val, denom;
		std::size_t ret_dim;
		boost::tie(val, denom, ret_dim) = this->get_val_denom_dir(state, dim);

		if (denom == 0)
		{
			if ((up && val > 0) || (!up && val < 0))
			{
				dest.set_union(std::move(src));
				src.clear();
			}
		}
		else
		{
			dest.set_union(src.remove_cut(ret_dim, -val/denom, up == (denom > 0)));
		}
	}

	// `state` must provide `operator[](std::size_t) -> value_type`.
	//
	// Removes as few parameters as possible from `p_down` and `p_up` so that
	//  1. for all `p` in `p_down` the derivation at state in the direction `dim` is negative,
	//  2. for all `p` in `p_up` the derivation at state in the direction `dim` is positive.
	template <typename State, typename PT, typename Tag>
	void fix_paramsets(paramset<PT, Tag> & p_down, paramset<PT, Tag> & p_up, State const & state, std::size_t dim) const
	{
		value_type val, denom;
		std::size_t ret_dim;
		boost::tie(val, denom, ret_dim) = this->get_val_denom_dir(state, dim);

		if (denom == 0)
		{
			if (val >= 0)
				p_down.clear();

			if (val <= 0)
				p_up.clear();
		}
		else
		{
			p_down.remove_cut(ret_dim, -val/denom, denom > 0);
			p_up.remove_cut(ret_dim, -val/denom, denom < 0);
		}
	}

	friend std::ostream & operator<<(std::ostream & o, model const & v)
	{
		for (std::size_t i = 0; i < v.m_equations.size(); ++i)
			o << "dv" << i << " = " << v.m_equations[i] << std::endl;
		return o;
	}


private:
	// `state` must provide `operator[](std::size_t) -> value_type`.
	template <typename State>
	boost::tuple<value_type, value_type, std::size_t> get_val_denom_dir(State const & state, std::size_t dim) const
	{
		expression<value_type> const & eq = m_equations[dim];
		value_type val = 0;

		value_type denom = 0;
		std::size_t ret_dim = dims();

		for (std::size_t i = 0; i < eq.size(); ++i)
		{
			value_type v = eq[i].k();
			for (std::size_t j = 0; j < eq[i].vars().size(); ++j)
			{
				if (eq[i].vars()[j])
					v *= state[j];
			}
			for (std::size_t j = 0; j < eq[i].m_ramps.size(); ++j)
				v *= eq[i].m_ramps[j].value(state[eq[i].m_ramps[j].dim]);

			if (eq[i].has_param())
			{
				BOOST_ASSERT(ret_dim == dims());
				ret_dim = eq[i].param();
				denom = v;
			}
			else
			{
				val += v;
			}
		}

		return boost::make_tuple(val, denom, ret_dim);
	}

	std::vector<expression<T> > m_equations;
};

#endif

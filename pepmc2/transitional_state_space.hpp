#ifndef PEPMC_TRANSITIONAL_STATE_SPACE_HPP
#define PEPMC_TRANSITIONAL_STATE_SPACE_HPP

#include "state_space.hpp"

template <typename T>
class transitional_state_space
{
public:
	typedef T value_type;

	typedef state_space<T> ss_t;
	ss_t & m_ss;

	transitional_state_space(ss_t & ss)
		: m_ss(ss)
	{
	}

	struct vertex_descriptor
	{
		typename ss_t::vertex_descriptor vertex;
		std::size_t dim;
		std::size_t above;
		std::size_t bellow;
		bool fair;

		vertex_descriptor()
			: dim(0), above(0), bellow(0), fair(true)
		{
		}

		bool empty() const
		{
			return vertex.empty();
		}

		friend std::ostream & operator<<(std::ostream & out, vertex_descriptor const & v)
		{
			return out << "{" << v.vertex << ", " << v.dim << ", "
				<< v.above << ", " << v.bellow << ", " << "UF"[v.fair] << "}";
		}

		friend bool operator==(vertex_descriptor const & lhs, vertex_descriptor const & rhs)
		{
			return boost::tie(lhs.vertex, lhs.dim, lhs.above, lhs.bellow, lhs.fair)
				== boost::tie(rhs.vertex, rhs.dim, rhs.above, rhs.bellow, rhs.fair);
		}

		friend bool operator<(vertex_descriptor const & lhs, vertex_descriptor const & rhs)
		{
			return boost::tie(lhs.vertex, lhs.dim, lhs.above, lhs.bellow, lhs.fair)
				< boost::tie(rhs.vertex, rhs.dim, rhs.above, rhs.bellow, rhs.fair);
		}

		friend std::size_t hash_value(vertex_descriptor const & v)
		{
			using boost::hash_value;
			std::size_t res = hash_value(v.vertex);
			boost::hash_combine(res, v.dim);
			boost::hash_combine(res, v.above);
			boost::hash_combine(res, v.bellow);
			boost::hash_combine(res, v.fair);
			return res;
		}
	};

	bool final(vertex_descriptor const & v) const
	{
		return v.fair;
	}

	class init_enumerator
	{
	public:
		init_enumerator(transitional_state_space const & g)
			: m_g(g), m_inner(g.m_ss)
		{
		}

		bool valid() const
		{
			return m_inner.valid();
		}

		void next()
		{
			m_inner.next();
		}

		vertex_descriptor get() const
		{
			vertex_descriptor res;
			res.vertex = m_inner.get();
			res.fair = m_g.m_ss.final(res.vertex);
			return res;
		}

	private:
		transitional_state_space const & m_g;
		typename ss_t::init_enumerator m_inner;
	};

	class out_edge_enumerator
	{
	public:
		out_edge_enumerator(transitional_state_space const & g, vertex_descriptor const & source)
			: m_g(g), m_source(source), m_inner(g.m_ss, source.vertex)
		{
			this->update_target();
		}

		void reset()
		{
			m_inner.reset();
		}

		vertex_descriptor const & source() const
		{
			return m_source;
		}

		vertex_descriptor const & target() const
		{
			return m_target;
		}

		template <typename Tag>
		void paramset_intersect(paramset<T, Tag> & p) const
		{
			m_inner.paramset_intersect(p);
			if (m_target.above + m_target.bellow == 0)
				return;

			paramset<T, Tag> p_transient;
			for (std::size_t dim = 0; dim < m_g.m_ss.dims(); ++dim)
			{
				paramset<T, Tag> p_down = p;
				paramset<T, Tag> p_up = p;

				auto v = m_target.vertex;

				m_g.m_ss.remove_nontransient(p_down, p_up, v, dim);
				for (std::size_t i = 0; i < m_target.above; ++i)
				{
					v.coords[m_target.dim] = m_target.vertex.coords[m_target.dim] + i + 1;
					m_g.m_ss.remove_nontransient(p_down, p_up, v, dim);
				}
				for (std::size_t i = 0; i < m_target.bellow; ++i)
				{
					v.coords[m_target.dim] = m_target.vertex.coords[m_target.dim] - i - 1;
					m_g.m_ss.remove_nontransient(p_down, p_up, v, dim);
				}

				p_transient.set_union(std::move(p_down));
				p_transient.set_union(std::move(p_up));
			}

			if (!m_target.fair)
				p = std::move(p_transient);
			else
				p.set_difference(p_transient);
		}

		bool valid() const
		{
			return m_inner.valid();
		}

		void next()
		{
			if (!m_target.fair && m_g.m_ss.final(m_target.vertex))
			{
				m_target.fair = true;
				return;
			}

			m_inner.next();
			this->update_target();
		}

	private:
		void update_target()
		{
			if (!m_inner.valid())
				return;

			m_target.vertex = m_inner.target();

			// TODO: Make the dependency a little less explicit.
			m_target.dim = m_inner.m_dim;

			if (m_source.dim != m_inner.m_dim)
			{
				m_target.above = 0;
				m_target.bellow = 0;
				m_target.fair = m_g.m_ss.final(m_target.vertex);
			}
			else if (!m_inner.m_up)
			{
				m_target.above = m_source.above + 1;
				m_target.bellow = (m_source.bellow == 0? 0: m_source.bellow - 1);
				m_target.fair = false;
			}
			else 
			{
				m_target.bellow = m_source.bellow + 1;
				m_target.above = (m_source.above == 0? 0: m_source.above - 1);
				m_target.fair = false;
			}
		}

		transitional_state_space const & m_g;
		vertex_descriptor m_source;
		vertex_descriptor m_target;
		typename ss_t::out_edge_enumerator m_inner;
	};

	template <typename Paramset>
	void self_loop(vertex_descriptor const & v, Paramset & p) const
	{
		m_ss.self_loop(v.vertex, p);
	}

	typedef typename ss_t::literal_proposition literal_proposition;
	typedef typename ss_t::proposition proposition;

	bool valid(proposition const & prop, vertex_descriptor const & v) const
	{
		return m_ss.valid(prop, v.vertex);
	}

	void refine_thresholds(std::size_t amount)
	{
		m_ss.refine_thresholds(amount);
	}

	friend std::ostream & operator<<(std::ostream & o, transitional_state_space const & v)
	{
		return o << v.m_ss;
	}
};

#endif

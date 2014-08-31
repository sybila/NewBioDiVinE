#ifndef BUCHI_AUTOMATON_HPP
#define BUCHI_AUTOMATON_HPP

#include <vector>
#include <boost/functional/hash.hpp>
#include <boost/tuple/tuple_comparison.hpp>

template <typename Proposition>
struct buchi_automaton
{
	typedef Proposition proposition_type;

	struct state
	{
		std::vector<std::pair<proposition_type, std::size_t> > next;
		bool final;
	};

	bool final(std::size_t state) const
	{
		return m_states.at(state).final;
	}

	std::vector<state> m_states;
};

template <typename Proposition, typename PKS>
class ba_sync_product
{
public:
	typedef typename PKS::value_type value_type;

	ba_sync_product(buchi_automaton<Proposition> const & ba, PKS const & pks)
		: m_ba(ba), m_pks(pks)
	{
	}

	struct vertex_descriptor
	{
		typename PKS::vertex_descriptor vertex;
		std::size_t state;
		bool fair_left;
		bool fair_right;

		bool empty() const
		{
			return vertex.empty();
		}

		friend std::ostream & operator<<(std::ostream & out, vertex_descriptor const & rhs)
		{
			return out << "(" << rhs.vertex << ", " << rhs.state << ", " << "0LRB"[rhs.fair_right * 2 + rhs.fair_left] << ")";
		}

		friend bool operator==(vertex_descriptor const & lhs, vertex_descriptor const & rhs)
		{
			return lhs.state == rhs.state && lhs.vertex == rhs.vertex && lhs.fair_left == rhs.fair_left && lhs.fair_right == rhs.fair_right;
		}

		bool operator<(vertex_descriptor const & rhs) const
		{
			return boost::tie(state, vertex, fair_left, fair_right) < boost::tie(rhs.state, rhs.vertex, rhs.fair_left, rhs.fair_right);
		}

		friend std::size_t hash_value(vertex_descriptor const & v)
		{
			std::size_t res = hash_value(v.vertex);
			boost::hash_combine(res, v.state);
			boost::hash_combine(res, v.fair_left);
			boost::hash_combine(res, v.fair_right);
			return res;
		}
	};

	bool final(vertex_descriptor const & v) const
	{
		return v.fair_left && v.fair_right;
	}

	class init_enumerator
	{
	public:
		explicit init_enumerator(ba_sync_product const & g)
			: m_g(g), m_pks_init(m_g.m_pks)
		{
		}

		bool valid() const
		{
			return m_pks_init.valid();
		}

		void next()
		{
			m_pks_init.next();
		}

		vertex_descriptor get() const
		{
			typename PKS::vertex_descriptor const & pksv = m_pks_init.get();	
			vertex_descriptor v = { pksv, 0, m_g.m_pks.final(pksv), m_g.m_ba.final(0) };
			return v;
		}
	private:
		ba_sync_product const & m_g;
		typename PKS::init_enumerator m_pks_init;
	};

	class out_edge_enumerator
	{
	public:
		out_edge_enumerator(ba_sync_product const & g, vertex_descriptor const & source)
			: m_g(g), m_out_edges(g.m_pks, source.vertex), m_source(source)
		{
			this->reset();
		}

		vertex_descriptor const & source() const
		{
			return m_source;
		}

		vertex_descriptor target() const
		{
			vertex_descriptor res = { m_out_edges.target(), m_g.m_ba.m_states[m_source.state].next[m_state_index].second, m_source.fair_left, m_source.fair_right };

			if (res.fair_left && res.fair_right)
				res.fair_left = res.fair_right = false;

			if (!res.fair_left)
				res.fair_left = m_g.m_pks.final(res.vertex);
			if (!res.fair_right)
				res.fair_right = m_g.m_ba.final(res.state);

			return res;
		}

		template <typename Paramset>
		void paramset_intersect(Paramset & p) const
		{
			m_out_edges.paramset_intersect(p);
		}

		bool valid() const
		{
			return m_state_index != m_g.m_ba.m_states[m_source.state].next.size();
		}

		void next()
		{
			BOOST_ASSERT(m_out_edges.valid());

			m_out_edges.next();
			if (!m_out_edges.valid())
			{
				++m_state_index;
				this->find_next();
				m_out_edges.reset();
			}
		}

		void reset()
		{
			m_out_edges.reset();
			m_state_index = 0;
			this->find_next();
		}

	private:
		void find_next()
		{
			while (m_state_index != m_g.m_ba.m_states[m_source.state].next.size()
				&& !m_g.m_pks.valid(m_g.m_ba.m_states[m_source.state].next[m_state_index].first, m_source.vertex))
			{
				++m_state_index;
			}
		}

		ba_sync_product const & m_g;
		typename PKS::out_edge_enumerator m_out_edges;
		vertex_descriptor m_source;
		std::size_t m_state_index;
	};

	template <typename Paramset>
	void self_loop(vertex_descriptor const & v, Paramset & p) const
	{
		if (!m_ba.final(v.state))
			p.clear();
		else
			m_pks.self_loop(v.vertex, p);
	}

private:
	buchi_automaton<Proposition> const & m_ba;
	PKS const & m_pks;
};

#endif

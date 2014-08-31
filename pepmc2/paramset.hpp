#ifndef PARAMSET_HPP
#define PARAMSET_HPP

#include <vector>
#include <utility>
#include <boost/assert.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/none.hpp>

template <typename T, typename Tag = boost::none_t>
class paramset
{
public:
	typedef T value_type;
	typedef Tag tag_type;

	struct region_t
	{
		region_t()
		{
		}

		template <typename ConstIterator>
		region_t(ConstIterator first, ConstIterator last)
			: box(first, last)
		{
		}

		template <typename ConstIterator, typename OTag>
		region_t(ConstIterator first, ConstIterator last, OTag && tag)
			: box(first, last), tag(std::forward<OTag>(tag))
		{
		}

		std::vector<std::pair<value_type, value_type> > box;
		tag_type tag;
	};

	void clear()
	{
		m_regions.clear();
	}

	bool empty() const
	{
		return m_regions.empty();
	}

	template <typename ConstIterator>
	void add_region(ConstIterator first, ConstIterator last)
	{
		region_t region(first, last);
		m_regions.push_back(std::move(region));
	}

	template <typename ConstIterator, typename OTag>
	void add_region(ConstIterator first, ConstIterator last, OTag && tag)
	{
		region_t region(first, last, std::forward<OTag>(tag));
		m_regions.push_back(std::move(region));
	}

	bool set_union(paramset && other)
	{
		return this->set_union_impl(std::move(other.m_regions));
	}

	bool set_union(paramset const & other)
	{
		return this->set_union_impl(other.m_regions);
	}

	void set_difference(paramset const & other)
	{
		std::vector<region_t> new_sregions;

		std::vector<region_t> const & oregions = other.regions();
		for (std::size_t oregion_index = 0; oregion_index < oregions.size(); ++oregion_index)
		{
			new_sregions.clear();

			region_t const & oregion = oregions[oregion_index];
			for (std::size_t sregion_index = 0; sregion_index < m_regions.size(); ++sregion_index)
			{
				region_t & sregion = m_regions[sregion_index];

				bool intersecting = true;
				for (std::size_t i = 0; intersecting && i < oregion.box.size(); ++i)
				{
					if (oregion.box[i].second <= sregion.box[i].first || sregion.box[i].second <= oregion.box[i].first)
						intersecting = false;
				}

				if (!intersecting)
				{
					new_sregions.push_back(std::move(sregion));
					continue;
				}

				for (std::size_t i = 0; i < oregion.box.size(); ++i)
				{
					region_t retained;

					boost::tie(retained, sregion) = split_region(std::move(sregion), i, oregion.box[i].first, false);
					if (!retained.box.empty())
						new_sregions.push_back(std::move(retained));
					if (sregion.box.empty())
						break;

					boost::tie(retained, sregion) = split_region(std::move(sregion), i, oregion.box[i].second, true);
					if (!retained.box.empty())
						new_sregions.push_back(std::move(retained));
					if (sregion.box.empty())
						break;
				}
			}

			m_regions = std::move(new_sregions);
		}
	}

	void set_intersection(paramset const & other)
	{
		std::vector<region_t> res;

		std::vector<region_t> new_sregions;

		std::vector<region_t> const & oregions = other.regions();
		for (std::size_t oregion_index = 0; oregion_index < oregions.size(); ++oregion_index)
		{
			new_sregions.clear();

			region_t const & oregion = oregions[oregion_index];
			for (std::size_t sregion_index = 0; sregion_index < m_regions.size(); ++sregion_index)
			{
				region_t & sregion = m_regions[sregion_index];

				bool intersecting = true;
				for (std::size_t i = 0; intersecting && i < oregion.box.size(); ++i)
				{
					if (oregion.box[i].second <= sregion.box[i].first || sregion.box[i].second <= oregion.box[i].first)
						intersecting = false;
				}

				if (!intersecting)
				{
					new_sregions.push_back(std::move(sregion));
					continue;
				}

				for (std::size_t i = 0; i < oregion.box.size(); ++i)
				{
					region_t removed;

					boost::tie(sregion, removed) = split_region(std::move(sregion), i, oregion.box[i].first, true);
					if (!removed.box.empty())
						new_sregions.push_back(std::move(removed));
					if (sregion.box.empty())
						break;

					boost::tie(sregion, removed) = split_region(std::move(sregion), i, oregion.box[i].second, false);
					if (!removed.box.empty())
						new_sregions.push_back(std::move(removed));
					if (sregion.box.empty())
						break;
				}

				if (!sregion.box.empty())
					res.push_back(std::move(sregion));
			}

			m_regions = std::move(new_sregions);
		}

		m_regions = std::move(res);
	}

	paramset remove_cut(std::size_t dim, value_type value, bool up)
	{
		paramset res;
		region_t removed;
		for (std::size_t i = m_regions.size(); i != 0; --i)
		{
			boost::tie(m_regions[i-1], removed) = split_region(std::move(m_regions[i-1]), dim, value, !up);
			if (m_regions[i-1].box.empty())
			{
				std::swap(m_regions[i-1], m_regions.back());
				m_regions.pop_back();
			}

			if (!removed.box.empty())
				res.m_regions.push_back(std::move(removed));
		}

		return std::move(res);
	}

	std::vector<region_t> const & regions() const
	{
		return m_regions;
	}

	std::vector<region_t> & regions()
	{
		return m_regions;
	}

	template <typename OTag>
	void retag(OTag && tag)
	{
		for (std::size_t i = 0; i < m_regions.size(); ++i)
			m_regions[i].tag = std::forward<OTag>(tag);
	}

	friend std::ostream & operator<<(std::ostream & out, paramset const & v)
	{
		for (std::size_t i = 0; i < v.m_regions.size(); ++i)
		{
			out << "[";
			for (std::size_t j = 0; j < v.m_regions[i].box.size(); ++j)
				out << "(" << v.m_regions[i].box[j].first << ", " << v.m_regions[i].box[j].second << ")";
			out << "]";
		}
		return out;
	}

private:
	bool set_union_impl(std::vector<region_t> oregions)
	{
		std::vector<region_t> new_oregions;
		for (std::size_t sregion_index = 0; sregion_index < m_regions.size(); ++sregion_index)
		{
			region_t const & sregion = m_regions[sregion_index];

			new_oregions.clear();
			for (std::size_t oregion_index = 0; oregion_index < oregions.size(); ++oregion_index)
			{
				region_t & oregion = oregions[oregion_index];

				BOOST_ASSERT(oregion.box.size() == sregion.box.size());

				bool intersecting = true;
				for (std::size_t i = 0; intersecting && i < oregion.box.size(); ++i)
				{
					if (oregion.box[i].second <= sregion.box[i].first || sregion.box[i].second <= oregion.box[i].first)
						intersecting = false;
				}

				if (!intersecting)
				{
					new_oregions.push_back(std::move(oregion));
					continue;
				}

				for (std::size_t i = 0; i < oregion.box.size(); ++i)
				{
					region_t removed;

					boost::tie(oregion, removed) = split_region(std::move(oregion), i, sregion.box[i].first, true);
					if (!removed.box.empty())
						new_oregions.push_back(std::move(removed));
					if (oregion.box.empty())
						break;

					boost::tie(oregion, removed) = split_region(std::move(oregion), i, sregion.box[i].second, false);
					if (!removed.box.empty())
						new_oregions.push_back(std::move(removed));
					if (oregion.box.empty())
						break;
				}
			}

			oregions = std::move(new_oregions);
		}

		for (std::size_t i = 0; i < oregions.size(); ++i)
			m_regions.push_back(std::move(oregions[i]));
		return !oregions.empty();
	}

	static std::pair<region_t, region_t> split_region(region_t && r, std::size_t dim, value_type value, bool up)
	{
		if ((up && r.box[dim].second <= value) || (!up && r.box[dim].first >= value))
			return std::make_pair(region_t(), std::move(r));

		if ((up && r.box[dim].first >= value) || (!up && r.box[dim].second <= value))
			return std::make_pair(std::move(r), region_t());

		region_t retained(std::move(r));
		region_t removed(retained);
		retained.box[dim].first = value;
		removed.box[dim].second = value;

		if (up)
			return std::make_pair(std::move(retained), std::move(removed));
		else
			return std::make_pair(std::move(removed), std::move(retained));
	}

	std::vector<region_t> m_regions;
};

#endif

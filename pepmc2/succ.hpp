#ifndef SUCC_HPP
#define SUCC_HPP

#include <vector>
#include <map>

#include "semaphore.hpp"
#include "sync_counter.hpp"
#include "barrier.hpp"

#include <boost/thread.hpp>

#ifdef PEPMC_LINUX_SETAFFINITY
# include <sys/types.h>
# include <sched.h>
# include <sys/syscall.h>
pid_t gettid()
{
	return syscall(SYS_gettid);
}
#endif

class succ
{
public:
	explicit succ(std::size_t thread_count)
		: m_thread_count(thread_count)
	{
	}

	template <typename Structure, typename Paramset, typename Crop>
	struct impl
	{
		typedef std::pair<typename Structure::vertex_descriptor, Paramset> message_t;
		typedef std::vector<message_t> message_queue_t;

		impl(Structure const & s, std::size_t thread_count, Crop & crop)
			: s(s), res(thread_count), active_processes(0), crop(crop), m_barrier(thread_count)
		{
			event_queues.resize(thread_count, std::vector<message_queue_t *>(thread_count));
			for (std::size_t i = 0; i < thread_count; ++i)
			{
				for (std::size_t j = 0; j < thread_count; ++j)
					event_queues[i][j] = new message_queue_t();
			}
		}

		~impl()
		{
			for (std::size_t i = 0; i < event_queues.size(); ++i)
			{
				for (std::size_t j = 0; j < event_queues[i].size(); ++j)
					delete event_queues[i][j];
			}
		}

		template <typename V, typename P>
		void append_queue(V && v, P && p)
		{
			active_processes.add(1);
			std::size_t id = hash_value(v) % res.size();
			event_queues[id][0]->push_back(std::make_pair(std::forward<V>(v), std::forward<P>(p)));
		}

		Structure const & s;
		std::vector<std::map<typename Structure::vertex_descriptor, Paramset> > res;

		// event_queues[i][j] -- messages from j to i
		std::vector<std::vector<message_queue_t *> > event_queues;
		barrier m_barrier;
		sync_counter active_processes;
		Crop & crop;
	};

	template <typename Structure, typename Paramset, typename Crop>
	struct queue_set
	{
		typedef std::pair<typename Structure::vertex_descriptor, Paramset> message_t;
		typedef std::vector<message_t> message_queue_t;

		queue_set(std::size_t id, impl<Structure, Paramset, Crop> & im)
			: id(id), im(im), queues(im.event_queues.size())
		{
			for (std::size_t i = 0; i < queues.size(); ++i)
				queues[i] = new message_queue_t;
		}

		~queue_set()
		{
			for (std::size_t i = 0; i < queues.size(); ++i)
				delete queues[i];
		}

		void fetch(std::vector<message_queue_t *> & queues) const
		{
			std::vector<message_queue_t *> & q = im.event_queues[id];
			queues.swap(q);
		}

		void post(std::vector<message_queue_t *> & in_queues)
		{
			for (std::size_t i = 0; i < queues.size(); ++i)
			{
				im.event_queues[i][id] = queues[i];
				queues[i] = 0;
			}
			in_queues.swap(queues);
		}

		template <typename V, typename P>
		void append_queue(V && v, P && p)
		{
			std::size_t id = hash_value(v) % queues.size();
			queues[id]->push_back(std::make_pair(std::forward<V>(v), std::forward<P>(p)));
		}

		std::size_t id;
		impl<Structure, Paramset, Crop> & im;
		std::vector<message_queue_t *> queues;
	};

	template <typename Structure, typename Paramset, typename Crop>
	static std::size_t worker(impl<Structure, Paramset, Crop> & im, std::size_t id)
	{
#ifdef PEPMC_LINUX_SETAFFINITY
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(id, &cpuset);
		sched_setaffinity(gettid(), sizeof cpuset, &cpuset);
#endif

		std::size_t bfs_levels = 0;

		typedef std::pair<typename Structure::vertex_descriptor, Paramset> message_t;
		typedef std::vector<message_t> message_queue_t;

		queue_set<Structure, Paramset, Crop> qs(id, im);
		std::vector<message_queue_t *> in_queues(im.event_queues.size());

		std::map<typename Structure::vertex_descriptor, Paramset> & res = im.res[id];

		for (; im.active_processes.get() != 0; ++bfs_levels)
		{
			qs.fetch(in_queues);

			im.m_barrier.wait();

			std::map<typename Structure::vertex_descriptor, Paramset> results;

			for (std::size_t i = 0; i < in_queues.size(); ++i)
			{
				message_queue_t & q = *in_queues[i];
				im.active_processes.sub(q.size());

				for (std::size_t j = 0; j < q.size(); ++j)
				{
					message_t & m = q[j];

					typename Structure::vertex_descriptor const & u = m.first;
					Paramset const & pu_add = m.second;
					Paramset & pu = res[u];
					if (!pu.set_union(pu_add))
						continue;

					typename Structure::out_edge_enumerator out_edges(im.s, u);
					for (; out_edges.valid(); out_edges.next())
					{
						typename Structure::vertex_descriptor const & v = out_edges.target();
						Paramset puv = pu;
						out_edges.paramset_intersect(puv);
						im.crop(v, puv);
						if (!puv.empty())
						{
							puv.retag(out_edges.source());
							results[v].set_union(std::move(puv));
						}
					}
				}

				q.clear();
			}

			for (auto ci = results.begin(); ci != results.end(); ++ci)
				qs.append_queue(ci->first, std::move(ci->second));

			im.active_processes.add(results.size());
			qs.post(in_queues);

			im.m_barrier.wait();
		}

		return bfs_levels;
	}

	template <typename Structure, typename InitEnum, typename Paramset, typename Crop, typename Visitor>
	std::map<typename Structure::vertex_descriptor, Paramset> operator()(
		Structure const & s,
		InitEnum init_enum,
		Paramset const & p,
		Crop crop,
		Visitor & visitor)
	{
		impl<Structure, Paramset, Crop> im(s, m_thread_count, crop);
		
		for (; init_enum.valid(); init_enum.next())
		{
			typename Structure::out_edge_enumerator out_edges(s, init_enum.get());
			for (; out_edges.valid(); out_edges.next())
			{
				typename Structure::vertex_descriptor const & v = out_edges.target();

				Paramset puv = p;
				out_edges.paramset_intersect(puv);
				crop(v, puv);
				puv.retag(out_edges.source());

				im.append_queue(v, puv);
			}
		}


		std::vector<boost::thread> th;
		for (std::size_t i = 1; i < m_thread_count; ++i)
			th.push_back(boost::thread(boost::bind(&worker<Structure, Paramset, Crop>, boost::ref(im), i)));
		std::size_t bfs_levels = worker(im, 0);
		visitor.succ_bfs_levels(bfs_levels);
		for (std::size_t i = 0; i < th.size(); ++i)
			th[i].join();
		std::map<typename Structure::vertex_descriptor, Paramset> res;
		for (std::size_t i = 0; i < im.res.size(); ++i)
			res.insert(im.res[i].begin(), im.res[i].end());
		return res;
	}

private:
	std::size_t m_thread_count;
};

#endif

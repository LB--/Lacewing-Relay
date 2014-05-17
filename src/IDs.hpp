#ifndef IdManagement_HeaderPlusPlus
#define IdManagement_HeaderPlusPlus
#include <set>
#include <limits>

template<typename T>
struct IdManager final
{
	using ID_type = T;
	ID_type generate() noexcept
	{
		ID_type ret (lowest);
		while(IDs.find(++lowest) != IDs.end())
		{
		}
		return IDs.insert(ret), ret;
	}
	void release(ID_type ID) noexcept
	{
		IDs.erase(ID);
		lowest = (ID < lowest) ? ID : lowest;
	}

private:
	std::set<ID_type> IDs;
	ID_type lowest = std::numeric_limits<ID_type>::min();
};
template<typename T>
struct IdHolder final
{
	IdManager<T> &manager;
	T const id;

	IdHolder(IdManager<T> &idm) noexcept
	: manager(idm)
	, id(idm.generate())
	{
	}
	~IdHolder()
	{
		manager.release(id);
	}

	operator T() const noexcept
	{
		return id;
	}
};

#endif

#pragma once
#include <unordered_map>
#include <vector>
#include <utility> // for std::forward
#include <functional>

namespace etc
{
	// https://github.com/juanchopanza/cppblog/tree/master/Patterns/Observer
	template <typename Event, typename Argument>
	struct EventHandler
	{
		EventHandler() = default;

		template <typename Observer>
		void Register(const Event& event, Observer&& observer)
		{
			mObservers[event].push_back(std::forward<Observer>(observer));
		}

		template <typename Observer>
		void Register(Event&& event, Observer&& observer)
		{
			mObservers[std::move(event)].push_back(std::forward<Observer>(observer));
		}

		void Notify(const Event& event, const Argument& arg) const
		{
			const auto iter = mObservers.find(event);
			if (iter != mObservers.end())
				for (const auto& observer : iter->second)
					observer(arg);
		}

		// disallow copying and assigning
		EventHandler(const EventHandler&) = delete;
		EventHandler& operator=(const EventHandler&) = delete;

	private:
		std::unordered_map<Event, std::vector<std::function<void(const Argument&)> > > mObservers;
	};

} // core
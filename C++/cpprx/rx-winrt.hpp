#pragma once
#include "rx-includes.hpp"

#if !defined(CPPRX_RX_WINRT_HPP)
#define CPPRX_RX_WINRT_HPP
#pragma once

#if RXCPP_USE_WINRT

#define NOMINMAX
#include <Windows.h>

namespace rxcpp { namespace winrt {
    namespace wf = Windows::Foundation;
    namespace wuicore = Windows::UI::Core;
    namespace wuixaml = Windows::UI::Xaml;

    template <class TSender, class TEventArgs>
    struct EventPattern
    {
        EventPattern(TSender sender, TEventArgs eventargs) :
            sender(sender),
            eventargs(eventargs)
        {}

        TSender Sender() {
            return sender;};
        TEventArgs EventArgs() {
            return eventargs;};

    private:
        TSender sender;
        TEventArgs eventargs;
    };

    namespace detail
    {
        template <class SpecificTypedEventHandler>
        struct is_typed_event_handler : public std::false_type {};

        template <class Sender, class EventArgs>
        struct is_typed_event_handler<wf::TypedEventHandler<Sender, EventArgs>> : public std::true_type {};

        template <class SpecificTypedEventHandler>
        struct get_sender;
        template <class Sender, class EventArgs>
        struct get_sender<wf::TypedEventHandler<Sender, EventArgs>>
        {
            typedef Sender type;
        };

        template <class SpecificTypedEventHandler>
        struct get_eventargs;
        template <class Sender, class EventArgs>
        struct get_eventargs<wf::TypedEventHandler<Sender, EventArgs>>
        {
            typedef EventArgs type;
        };

        template <class SpecificTypedEventHandler>
        struct get_eventpattern;
        template <class Sender, class EventArgs>
        struct get_eventpattern<wf::TypedEventHandler<Sender, EventArgs>>
        {
            typedef EventPattern<Sender, EventArgs> type;
        };

        template<class T>
        struct remove_ref { typedef T type; };
        template<class T>
        struct remove_ref<T^> { typedef T type; };
        template<class T>
        struct remove_ref<T^ const> { typedef T type; };
        template<class T>
        struct remove_ref<T^ const &> { typedef T type; };
    }

    template <class EventHandler, class EventArgs>
    auto FromEventPattern(
        std::function<wf::EventRegistrationToken(EventHandler^)> addHandler,
        std::function<void (wf::EventRegistrationToken)> removeHandler)
        -> typename std::enable_if < !detail::is_typed_event_handler<EventHandler>::value, std::shared_ptr < Observable < EventPattern < Platform::Object^, EventArgs^ >> >> ::type
    {
        typedef EventPattern<Platform::Object^, EventArgs^> EP;
        return CreateObservable<EP>(
            [=](std::shared_ptr<Observer<EP>> observer)
            {
                auto h = ref new EventHandler(
                    [=](Platform::Object^ sender, EventArgs^ args) -> void
                    {
                        observer->OnNext(EP(sender, args));
                    });

                auto token = addHandler(h);

                return Disposable(
                    [removeHandler, token]()
                {
                    removeHandler(token);
                });
        });
    }

    template <class EventHandler, class Sender, class EventArgs>
    auto FromEventPattern(
        std::function<wf::EventRegistrationToken(EventHandler^)> addHandler,
        std::function<void (wf::EventRegistrationToken)> removeHandler)
        -> typename std::enable_if < !detail::is_typed_event_handler<EventHandler>::value, std::shared_ptr < Observable < EventPattern < Sender^, EventArgs^ >> >> ::type
    {
        typedef EventPattern<Sender^, EventArgs^> EP;
        return CreateObservable<EP>(
            [=](std::shared_ptr<Observer<EP>> observer)
            {
                auto h = ref new EventHandler(
                    [=](Platform::Object^ sender, EventArgs^ args) -> void
                    {
                        observer->OnNext(EP(dynamic_cast<Sender^>(sender), args));
                    });

                auto token = addHandler(h);

                return Disposable(
                    [removeHandler, token]()
                {
                    removeHandler(token);
                });
        });
    }

    template <class SpecificTypedEventHandler>
    auto FromEventPattern(
        std::function<wf::EventRegistrationToken(SpecificTypedEventHandler^)> addHandler,
        std::function<void (wf::EventRegistrationToken)> removeHandler)
        -> typename std::enable_if < detail::is_typed_event_handler<SpecificTypedEventHandler>::value, std::shared_ptr < Observable < typename detail::get_eventpattern<SpecificTypedEventHandler>::type> >> ::type
    {
        typedef typename detail::get_eventpattern<SpecificTypedEventHandler>::type EP;
        return CreateObservable<EP>(
            [=](std::shared_ptr < Observer < EP >> observer)
        {
            auto h = ref new SpecificTypedEventHandler(
                [=](typename detail::get_sender<SpecificTypedEventHandler>::type sender, typename detail::get_eventargs<SpecificTypedEventHandler>::type args) -> void
                {
                    observer->OnNext(EP(sender, args));
                });

            auto token = addHandler(h);

            return Disposable(
                [removeHandler, token]()
                {
                    removeHandler(token);
                });
        });
    }

    struct CoreDispatcherScheduler : public LocalScheduler
    {
    private:
        CoreDispatcherScheduler(const CoreDispatcherScheduler&);

    public:
        CoreDispatcherScheduler(wuicore::CoreDispatcher^ dispatcher, wuicore::CoreDispatcherPriority priority = wuicore::CoreDispatcherPriority::Normal)
            : dispatcher(dispatcher)
            , priority(priority)
        {
        }
        virtual ~CoreDispatcherScheduler()
        {
        }

        typedef std::shared_ptr<CoreDispatcherScheduler> shared;

        static shared Current()
        {
            auto window = wuixaml::Window::Current;
            if (window == nullptr)
            {
                throw std::logic_error("No window current");
            }
            return std::make_shared<CoreDispatcherScheduler>(window->Dispatcher);
        }

        wuicore::CoreDispatcher^ Dispatcher() 
        {
            return dispatcher;
        }

        wuicore::CoreDispatcherPriority Priority()
        {
            return priority;
        }

        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            // TODO: Build DispatchTimer instead
            Scheduler::shared sched;
            if (CurrentThreadScheduler::IsScheduleRequired())
            {
                if (Now() > dueTime || (dueTime - Now()) < std::chrono::milliseconds(500))
                {
                    sched = std::make_shared<ImmediateScheduler>();
                }
                else
                {
                    sched = std::make_shared<EventLoopScheduler>();
                }
            }
            else
            {
                sched = std::make_shared<CurrentThreadScheduler>();
            }
            auto that = shared_from_this();
            return sched->Schedule(dueTime, 
                [this, that, work](Scheduler::shared sched) mutable -> Disposable
                {
                    dispatcher->RunAsync(
                        priority,
                        ref new wuicore::DispatchedHandler(
                            [that, this, work]() mutable
                            {
                                this->Do(work, that);
                            },
                            Platform::CallbackContext::Any)
                    );
                    return Disposable::Empty();
                });
        }

    private:
        wuicore::CoreDispatcher^ dispatcher;
        wuicore::CoreDispatcherPriority priority;
    };

} }

#endif

#endif

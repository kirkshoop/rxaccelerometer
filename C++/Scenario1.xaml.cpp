//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

//
// Scenario1.xaml.cpp
// Implementation of the Scenario1 class
//

#include "pch.h"
#include "Scenario1.xaml.h"

using namespace SDKSample::AccelerometerCPP;

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Platform;

Scenario1::Scenario1() : 
    rootPage(MainPage::Current), 
    navigated(std::make_shared < rx::BehaviorSubject < bool >> (true)),
    desiredReportInterval(0)
{
    InitializeComponent();

    // start out disabled
    auto enabled = std::make_shared<rx::BehaviorSubject<bool>>(false);
    // start out not-working
    auto working = std::make_shared<rx::BehaviorSubject<bool>>(false);

    // use !enabled and !working to control canExecute
    enable = std::make_shared < rxrt::ReactiveCommand < RoutedEventPattern> >(observable(from(enabled)
        .combine_latest([](bool e, bool w)
        {
            return !e && !w; 
        }, working)));
    // use enabled and !working to control canExecute
    disable = std::make_shared < rxrt::ReactiveCommand < RoutedEventPattern> >(observable(from(enabled)
        .combine_latest([](bool e, bool w)
        {
            return e && !w; 
        }, working)));

    // when enable or disable is executing mark as working (both commands should be disabled)
    observable(from(enable->IsExecuting())
        .combine_latest([](bool ew, bool dw)
        {
            return ew || dw; 
        }, disable->IsExecuting()))
        ->Subscribe(observer(working));

    // when enable is executed mark the scenario enabled, when disable is executed mark the scenario disabled
    observable(from(observable(enable))
        .select([this](RoutedEventPattern)
        {
            return this->accelerometer != nullptr; 
        })
        .merge(observable(from(observable(disable))
            .select([](RoutedEventPattern)
            {
                return false; 
            }))))
        ->Subscribe(observer(enabled)); // this is a subscription to the enable and disable ReactiveCommands

    typedef TypedEventHandler<Accelerometer^, AccelerometerReadingChangedEventArgs^> AccelerometerReadingChangedTypedEventHandler;
    auto readingChanged = from(rxrt::FromEventPattern<AccelerometerReadingChangedTypedEventHandler>(
        [this](AccelerometerReadingChangedTypedEventHandler^ h)
        {
            return this->accelerometer->ReadingChanged += h;
        },
        [this](Windows::Foundation::EventRegistrationToken t)
        {
            this->accelerometer->ReadingChanged -= t;
        }))
        .select([](rxrt::EventPattern<Accelerometer^, AccelerometerReadingChangedEventArgs^> e)
        {
            // on sensor thread
            return e.EventArgs()->Reading;
        })
        // push readings to ui thread
        .observe_on_dispatcher()
        .publish()
        .ref_count();

    auto currentWindow = Window::Current;
    auto visiblityChanged = from(rxrt::FromEventPattern<WindowVisibilityChangedEventHandler, VisibilityChangedEventArgs>(
        [currentWindow](WindowVisibilityChangedEventHandler^ h)
        {
            return currentWindow->VisibilityChanged += h;
        },
        [currentWindow](Windows::Foundation::EventRegistrationToken t)
        {
            currentWindow->VisibilityChanged -= t;
        }))
        .select([](rxrt::EventPattern<Platform::Object^, VisibilityChangedEventArgs^> e)
        {
            return e.EventArgs()->Visible;
        })
        .publish()
        .ref_count();

    auto visible = from(visiblityChanged)
        .where([](bool v)
        {
            return !!v;
        }).merge(
        from(observable(navigated))
            .where([](bool n)
            {
                return n;
            }));

    auto invisible = from(visiblityChanged)
        .where([](bool v)
        {
            return !v;
        });

    // the scenario ends when:
    auto endScenario = 
        // - disable is executed
        from(observable(disable))
            .select([](RoutedEventPattern)
            {
                return true; 
            }).merge(
        // - the scenario is navigated from
        from(observable(navigated))
            .where([](bool n)
            {
                return !n; 
            }));

    from(enable->RegisterAsyncFunction(
        [](RoutedEventPattern)
        {
            // background thread
            // enable and disable commands will be disabled until this is finished

            std::this_thread::sleep_for(std::chrono::seconds(2));

            return true;
        })) // this is a subscription to the enable ReactiveCommand
        // back on the ui thread
        .subscribe([this](bool) // takes whatever was returned above
        {
            // update the ui
        });

    // enable the scenario when enable is executed
    from(observable(enable))
        .where([this](RoutedEventPattern)
        {
            return this->accelerometer != nullptr;
        })
        .select_many([=](RoutedEventPattern)
        {
            return from(visible)
                .select_many([=](bool)
                {
                    // enable sensor input 
                    return rx::from(readingChanged)
                        .take_until(invisible);
                })
                .take_until(endScenario); // this is a subscription to the disable ReactiveCommand
        })
        .subscribe([this](AccelerometerReading^ reading)
        {
            // on the ui thread
            this->ScenarioOutput_X->Text = reading->AccelerationX.ToString();
            this->ScenarioOutput_Y->Text = reading->AccelerationY.ToString();
            this->ScenarioOutput_Z->Text = reading->AccelerationZ.ToString();
        });

    rxrt::BindCommand(ScenarioEnableButton, enable);

    rxrt::BindCommand(ScenarioDisableButton, disable);

    accelerometer = Accelerometer::GetDefault();
    if (accelerometer != nullptr)
    {
        // Select a report interval that is both suitable for the purposes of the app and supported by the sensor.
        // This value will be used later to activate the sensor.
        uint32 minReportInterval = accelerometer->MinimumReportInterval;
        desiredReportInterval = minReportInterval > 16 ? minReportInterval : 16;

        // Establish the report interval
        accelerometer->ReportInterval = desiredReportInterval;
    }
    else
    {
        rootPage->NotifyUser("No accelerometer found", NotifyType::ErrorMessage);
    }
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void Scenario1::OnNavigatedTo(NavigationEventArgs^)
{
    navigated->OnNext(true);
}

/// <summary>
/// Invoked when this page is no longer displayed.
/// </summary>
/// <param name="e"></param>
void Scenario1::OnNavigatedFrom(NavigationEventArgs^)
{
    navigated->OnNext(false);
}

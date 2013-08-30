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
// Scenario3.xaml.cpp
// Implementation of the Scenario3 class
//

#include "pch.h"
#include "Scenario3.xaml.h"

using namespace SDKSample::AccelerometerCPP;

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Platform;

Scenario3::Scenario3() : 
    rootPage(MainPage::Current), 
    desiredReportInterval(0),
    dispatchIntervalSubscription(rx::Disposable::Empty()),
    visibilitySubscription(rx::Disposable::Empty())
{
    InitializeComponent();

    // start out disabled
    enabled = std::make_shared<rx::BehaviorSubject<bool>>(false);

    // use enabled to control canExecute
    enable = std::make_shared < rxrt::ReactiveCommand < RoutedEventPattern> >(observable(from(enabled).select([](bool b){return !b; })));
    disable = std::make_shared < rxrt::ReactiveCommand < RoutedEventPattern> >(observable(enabled));

    from(observable(enable))
        // stay on the ui thread
        .subscribe(
        [this](RoutedEventPattern)
        {
            if (accelerometer != nullptr)
            {
                auto dispatcherInterval = rxrt::DispatcherInterval(std::chrono::milliseconds(desiredReportInterval));

                auto currentWindow = Window::Current;
                auto visibilityChanged = rxrt::FromEventPattern<WindowVisibilityChangedEventHandler, VisibilityChangedEventArgs>(
                    [currentWindow](WindowVisibilityChangedEventHandler^ h)
                {
                    return currentWindow->VisibilityChanged += h;
                },
                    [currentWindow](Windows::Foundation::EventRegistrationToken t)
                {
                    currentWindow->VisibilityChanged -= t;
                });

                ///
                /// allow subscription to be called from multiple points
                ///
                auto subscribeDispatchInterval =
                    [this, dispatcherInterval]()
                {
                    this->dispatchIntervalSubscription.Dispose();
                    this->dispatchIntervalSubscription = rx::from(dispatcherInterval)
                        .subscribe([this](size_t)
                    {
                        // on the ui thread
                        AccelerometerReading^ reading = this->accelerometer->GetCurrentReading();
                        if (reading != nullptr)
                        {
                            this->ScenarioOutput_X->Text = reading->AccelerationX.ToString();
                            this->ScenarioOutput_Y->Text = reading->AccelerationY.ToString();
                            this->ScenarioOutput_Z->Text = reading->AccelerationZ.ToString();
                        }
                    });
                };
                // initial subscribe
                subscribeDispatchInterval();

                /// This is the event handler for VisibilityChanged events. You would register for these notifications
                /// if handling sensor data when the app is not visible could cause unintended actions in the app.
                this->visibilitySubscription = rx::from(visibilityChanged)
                    .subscribe(
                    [this, subscribeDispatchInterval](rxrt::EventPattern<Platform::Object^, VisibilityChangedEventArgs^> e)
                {
                    // The app should watch for VisibilityChanged events to disable and re-enable sensor input as appropriate
                    if (ScenarioDisableButton->IsEnabled)
                    {
                        if (e.EventArgs()->Visible)
                        {
                            // Re-enable sensor input (no need to restore the desired reportInterval... it is restored for us upon app resume)
                            subscribeDispatchInterval();
                        }
                        else
                        {
                            // Disable sensor input (no need to restore the default reportInterval... resources will be released upon app suspension)
                            this->dispatchIntervalSubscription.Dispose();
                        }
                    }
                });

                // Set the report interval to enable the sensor for polling
                accelerometer->ReportInterval = desiredReportInterval;
            }
            else
            {
                rootPage->NotifyUser("No accelerometer found", NotifyType::ErrorMessage);
            }
            // now the scenario is enabled
            this->enabled->OnNext(true);
        });

    rxrt::BindCommand(ScenarioEnableButton, enable);

    from(observable(disable))
        // stay on the ui thread
        .subscribe([this](RoutedEventPattern)
        {
            dispatchIntervalSubscription.Dispose();
            visibilitySubscription.Dispose();
            // now the scenario is disabled
            this->enabled->OnNext(false);
        });

    rxrt::BindCommand(ScenarioDisableButton, disable);

    accelerometer = Accelerometer::GetDefault();
    if (accelerometer != nullptr)
    {
        // Select a report interval that is both suitable for the purposes of the app and supported by the sensor.
        // This value will be used later to activate the sensor.
        uint32 minReportInterval = accelerometer->MinimumReportInterval;
        desiredReportInterval = minReportInterval > 16 ? minReportInterval : 16;
    }
    else
    {
        rootPage->NotifyUser("No accelerometer found", NotifyType::ErrorMessage);
    }
}

/// <summary>
/// Invoked when this page is no longer displayed.
/// </summary>
/// <param name="e"></param>
void Scenario3::OnNavigatedFrom(NavigationEventArgs^ e)
{
    visibilitySubscription.Dispose();
    dispatchIntervalSubscription.Dispose();
}

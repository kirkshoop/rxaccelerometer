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
    desiredReportInterval(0), 
    readingSubscription(rx::Disposable::Empty()), 
    visibilitySubscription(rx::Disposable::Empty())
{
    InitializeComponent();

    // start out disabled
    enabled = std::make_shared<rx::BehaviorSubject<bool>>(false);

    // use enabled to control canExecute
    enable = std::make_shared < rxrt::ReactiveCommand < RoutedEventPattern> >(observable(from(enabled).select([](bool b){return !b; })));
    disable = std::make_shared < rxrt::ReactiveCommand < RoutedEventPattern> >(observable(enabled));

#if 0
    // if there is something to do on a background thread
    enable->RegisterAsyncFunction(
        [this](RoutedEventPattern ep)
        {
            // background thread
            ...
            return ep; // or something else
        }))
        // back on the ui thread
        .subscribe(
        [this](RoutedEventPattern) // take whatever was returned above
        {
            ...
        });
#endif

    from(observable(enable))
        // stay on the ui thread
        .subscribe(
        [this](RoutedEventPattern)
        {
            if (accelerometer != nullptr)
            {
                // Establish the report interval
                accelerometer->ReportInterval = desiredReportInterval;

                typedef TypedEventHandler<Accelerometer^, AccelerometerReadingChangedEventArgs^> AccelerometerReadingChangedTypedEventHandler;
                auto readingChanged = rxrt::FromEventPattern<AccelerometerReadingChangedTypedEventHandler>(
                    [this](AccelerometerReadingChangedTypedEventHandler^ h)
                {
                    return this->accelerometer->ReadingChanged += h;
                },
                    [this](Windows::Foundation::EventRegistrationToken t)
                {
                    this->accelerometer->ReadingChanged -= t;
                });

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
                /// allow subscription to readings to be called from multiple points
                ///
                auto subscribeReadings =
                    [this, readingChanged]()
                {
                    this->readingSubscription.Dispose();
                    this->readingSubscription = rx::from(readingChanged)
                        .select([this](rxrt::EventPattern<Accelerometer^, AccelerometerReadingChangedEventArgs^> e)
                    {
                        // on the sensor thread
                        return e.EventArgs()->Reading;
                    })
                        .observe_on_dispatcher()
                        .subscribe([this](AccelerometerReading^ reading)
                    {
                        // on the ui thread
                        this->ScenarioOutput_X->Text = reading->AccelerationX.ToString();
                        this->ScenarioOutput_Y->Text = reading->AccelerationY.ToString();
                        this->ScenarioOutput_Z->Text = reading->AccelerationZ.ToString();
                    }
                    );
                };
                // initial subscribe
                subscribeReadings();

                /// This is the event handler for VisibilityChanged events. You would register for these notifications
                /// if handling sensor data when the app is not visible could cause unintended actions in the app.
                this->visibilitySubscription = rx::from(visibilityChanged)
                    .subscribe(
                    [this, subscribeReadings](rxrt::EventPattern<Platform::Object^, VisibilityChangedEventArgs^> e)
                {
                    // The app should watch for VisibilityChanged events to disable and re-enable sensor input as appropriate
                    if (ScenarioDisableButton->IsEnabled)
                    {
                        if (e.EventArgs()->Visible)
                        {
                            // Re-enable sensor input (no need to restore the desired reportInterval... it is restored for us upon app resume)
                            subscribeReadings();
                        }
                        else
                        {
                            // Disable sensor input (no need to restore the default reportInterval... resources will be released upon app suspension)
                            this->readingSubscription.Dispose();
                        }
                    }
                });
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
            readingSubscription.Dispose();
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
void Scenario1::OnNavigatedFrom(NavigationEventArgs^ e)
{
    readingSubscription.Dispose();
    visibilitySubscription.Dispose();
}


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
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void Scenario1::OnNavigatedTo(NavigationEventArgs^ e)
{
    ScenarioEnableButton->IsEnabled = true;
    ScenarioDisableButton->IsEnabled = false;
}

/// <summary>
/// Invoked when this page is no longer displayed.
/// </summary>
/// <param name="e"></param>
void Scenario1::OnNavigatedFrom(NavigationEventArgs^ e)
{
    if (ScenarioDisableButton->IsEnabled)
    {
        readingSubscription.Dispose();
        visibilitySubscription.Dispose();
        //Window::Current->VisibilityChanged::remove(visibilityToken);
        //accelerometer->ReadingChanged::remove(readingToken);

        // Restore the default report interval to release resources while the sensor is not in use
        accelerometer->ReportInterval = 0;
    }
}

void Scenario1::ScenarioEnable(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
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
        //readingToken = accelerometer->ReadingChanged::add(ref new TypedEventHandler<Accelerometer^, AccelerometerReadingChangedEventArgs^>(this, &Scenario1::ReadingChanged));

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
        //visibilityToken = Window::Current->VisibilityChanged::add(ref new WindowVisibilityChangedEventHandler(this, &Scenario1::VisibilityChanged));

        ScenarioEnableButton->IsEnabled = false;
        ScenarioDisableButton->IsEnabled = true;
    }
    else
    {
        rootPage->NotifyUser("No accelerometer found", NotifyType::ErrorMessage);
    }
}

void Scenario1::ScenarioDisable(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    readingSubscription.Dispose();
    visibilitySubscription.Dispose();
    //Window::Current->VisibilityChanged::remove(visibilityToken);
    //accelerometer->ReadingChanged::remove(readingToken);

    // Restore the default report interval to release resources while the sensor is not in use
    accelerometer->ReportInterval = 0;

    ScenarioEnableButton->IsEnabled = true;
    ScenarioDisableButton->IsEnabled = false;
}

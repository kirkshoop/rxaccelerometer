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
// Scenario2.xaml.cpp
// Implementation of the Scenario2 class
//

#include "pch.h"
#include "Scenario2.xaml.h"

using namespace SDKSample::AccelerometerCPP;

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Platform;

Scenario2::Scenario2() : 
    rootPage(MainPage::Current), 
    shakeCounter(0),
    shakenSubscription(rx::Disposable::Empty()),
    visibilitySubscription(rx::Disposable::Empty())
{
    InitializeComponent();

    accelerometer = Accelerometer::GetDefault();
    if (accelerometer == nullptr)
    {
        rootPage->NotifyUser("No accelerometer found", NotifyType::ErrorMessage);
    }
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void Scenario2::OnNavigatedTo(NavigationEventArgs^ e)
{
    ScenarioEnableButton->IsEnabled = true;
    ScenarioDisableButton->IsEnabled = false;
}

/// <summary>
/// Invoked when this page is no longer displayed.
/// </summary>
/// <param name="e"></param>
void Scenario2::OnNavigatedFrom(NavigationEventArgs^ e)
{
    if (ScenarioDisableButton->IsEnabled)
    {
        shakenSubscription.Dispose();
        visibilitySubscription.Dispose();
        //Window::Current->VisibilityChanged::remove(visibilityToken);
        //accelerometer->Shaken::remove(shakenToken);
    }
}

void Scenario2::ScenarioEnable(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (accelerometer != nullptr)
    {
        typedef TypedEventHandler<Accelerometer^, AccelerometerShakenEventArgs^> AccelerometerShakenTypedEventHandler;
        auto shaken = rxrt::FromEventPattern<AccelerometerShakenTypedEventHandler>(
            [this](AccelerometerShakenTypedEventHandler^ h)
        {
            return this->accelerometer->Shaken += h;
        },
            [this](Windows::Foundation::EventRegistrationToken t)
        {
            this->accelerometer->Shaken -= t;
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
        /// allow subscription to be called from multiple points
        ///
        auto subscribeShaken =
            [this, shaken]()
        {
            this->shakenSubscription.Dispose();
            this->shakenSubscription = rx::from(shaken)
                .select([this](rxrt::EventPattern<Accelerometer^, AccelerometerShakenEventArgs^> e)
                {
                    // on the sensor thread
                    return ++this->shakeCounter;
                })
                .observe_on_dispatcher()
                .subscribe([this](uint16 value)
                {
                    // on the ui thread
                    this->ScenarioOutputText->Text = value.ToString();
                }
            );
        };
        // initial subscribe
        subscribeShaken();
        //shakenToken = accelerometer->Shaken::add(ref new TypedEventHandler<Accelerometer^, AccelerometerShakenEventArgs^>(this, &Scenario2::Shaken));

        /// This is the event handler for VisibilityChanged events. You would register for these notifications
        /// if handling sensor data when the app is not visible could cause unintended actions in the app.
        this->visibilitySubscription = rx::from(visibilityChanged)
            .subscribe(
            [this, subscribeShaken](rxrt::EventPattern<Platform::Object^, VisibilityChangedEventArgs^> e)
        {
            // The app should watch for VisibilityChanged events to disable and re-enable sensor input as appropriate
            if (ScenarioDisableButton->IsEnabled)
            {
                if (e.EventArgs()->Visible)
                {
                    // Re-enable sensor input (no need to restore the desired reportInterval... it is restored for us upon app resume)
                    subscribeShaken();
                }
                else
                {
                    // Disable sensor input (no need to restore the default reportInterval... resources will be released upon app suspension)
                    this->shakenSubscription.Dispose();
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

void Scenario2::ScenarioDisable(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    shakenSubscription.Dispose();
    visibilitySubscription.Dispose();
    //Window::Current->VisibilityChanged::remove(visibilityToken);
    //accelerometer->Shaken::remove(shakenToken);

    ScenarioEnableButton->IsEnabled = true;
    ScenarioDisableButton->IsEnabled = false;
}

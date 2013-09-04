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
    navigated(std::make_shared < rx::BehaviorSubject < bool >> (true)),
    desiredReportInterval(0)
{
    InitializeComponent();

    // start out disabled
    auto enabled = std::make_shared<rx::BehaviorSubject<bool>>(false);
    auto working = std::make_shared<rx::BehaviorSubject<bool>>(false);

    // use !enabled and !working to control canExecute
    enable = std::make_shared < rxrt::ReactiveCommand < RoutedEventPattern> >(observable(from(enabled).combine_latest([](bool e, bool w){
        return !e && !w; }, working)));
    // use enabled and !working to control canExecute
    disable = std::make_shared < rxrt::ReactiveCommand < RoutedEventPattern> >(observable(from(enabled).combine_latest([](bool e, bool w){
        return e && !w; }, working)));

    // when enable or disable is executing mark as working (both commands should be disabled)
    observable(from(enable->IsExecuting())
        .combine_latest([](bool ew, bool dw){
            return ew || dw; }, disable->IsExecuting()))
        ->Subscribe(observer(working));

    // when enable is executed mark the scenario enabled, when disable is executed mark the scenario disabled
    observable(from(observable(enable))
        .select([](RoutedEventPattern){
            return true; })
        .merge(observable(from(observable(disable)).select([](RoutedEventPattern){
            return false; }))))
        ->Subscribe(observer(enabled));

    auto currentReading = from(rxrt::DispatcherInterval(std::chrono::milliseconds(desiredReportInterval)))
        .select([this](size_t)
        {
            return this->accelerometer->GetCurrentReading();
        })
        .where([](AccelerometerReading^ r)
        {
            return r != nullptr;
        })
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

    // enable the scenario when enable is executed
    from(observable(enable))
        // stay on the ui thread
        .where([this](RoutedEventPattern)
        {
            if (!accelerometer)
            {
                rootPage->NotifyUser("No accelerometer found", NotifyType::ErrorMessage);
                return false;
            }

            // Establish the report interval
            accelerometer->ReportInterval = desiredReportInterval;

            return true;
        })
        .select_many([=](RoutedEventPattern)
        {
            return from(visible)
                .select_many([=](bool)
                {
                    // enable sensor input 
                    return rx::from(currentReading)
                        .take_until(invisible);
                })
                .take_until(endScenario);
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
void Scenario3::OnNavigatedTo(NavigationEventArgs^)
{
    navigated->OnNext(true);
}

/// <summary>
/// Invoked when this page is no longer displayed.
/// </summary>
/// <param name="e"></param>
void Scenario3::OnNavigatedFrom(NavigationEventArgs^)
{
    navigated->OnNext(false);
}

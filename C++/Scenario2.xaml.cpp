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
    navigated(std::make_shared < rx::BehaviorSubject < bool >> (true)),
    shakeCounter(0)
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
        .select([this](RoutedEventPattern){
            return this->accelerometer != nullptr; })
        .merge(observable(from(observable(disable)).select([](RoutedEventPattern){
            return false; }))))
        ->Subscribe(observer(enabled));

    typedef TypedEventHandler<Accelerometer^, AccelerometerShakenEventArgs^> AccelerometerShakenTypedEventHandler;
    auto shaken = from(rxrt::FromEventPattern<AccelerometerShakenTypedEventHandler>(
        [this](AccelerometerShakenTypedEventHandler^ h)
        {
            return this->accelerometer->Shaken += h;
        },
        [this](Windows::Foundation::EventRegistrationToken t)
        {
            this->accelerometer->Shaken -= t;
        }))
        .select([this](rxrt::EventPattern<Accelerometer^, AccelerometerShakenEventArgs^> e)
        {
            // on the sensor thread
            return ++this->shakeCounter;
        })
        // push shaken to ui thread
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

    // enable the scenario when enable is executed
    from(observable(enable))
        // stay on the ui thread
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
                    return rx::from(shaken)
                        .take_until(invisible);
                })
                .take_until(endScenario);
        })
        .subscribe([this](uint16 value)
        {
            // on the ui thread
            this->ScenarioOutputText->Text = value.ToString();
        });

    rxrt::BindCommand(ScenarioEnableButton, enable);

    rxrt::BindCommand(ScenarioDisableButton, disable);

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
void Scenario2::OnNavigatedTo(NavigationEventArgs^)
{
    navigated->OnNext(true);
}

/// <summary>
/// Invoked when this page is no longer displayed.
/// </summary>
/// <param name="e"></param>
void Scenario2::OnNavigatedFrom(NavigationEventArgs^)
{
    navigated->OnNext(false);
}
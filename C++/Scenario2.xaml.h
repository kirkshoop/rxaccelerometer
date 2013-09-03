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
// Scenario2.xaml.h
// Declaration of the Scenario2 class
//

#pragma once

#include "pch.h"
#include "Scenario2.g.h"
#include "MainPage.xaml.h"

namespace SDKSample
{
    namespace AccelerometerCPP
    {
        /// <summary>
        /// An empty page that can be used on its own or navigated to within a Frame.
        /// </summary>
    	[Windows::Foundation::Metadata::WebHostHidden]
        public ref class Scenario2 sealed
        {
        public:
            Scenario2();
    
        protected:
            virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
    
        private:
            typedef rxrt::EventPattern<Object^, Windows::UI::Xaml::RoutedEventArgs^> RoutedEventPattern;

            MainPage^ rootPage;
            Windows::UI::Core::CoreDispatcher^ dispatcher;
            Windows::Devices::Sensors::Accelerometer^ accelerometer;
            rxrt::ReactiveCommand<RoutedEventPattern>::shared enable;
            rxrt::ReactiveCommand<RoutedEventPattern>::shared disable;
            rx::Disposable shakenSubscription;
            rx::Disposable visibilitySubscription;
            uint16 shakeCounter;
        };
    }
}

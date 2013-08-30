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
// Scenario1.xaml.h
// Declaration of the Scenario1 class
//

#pragma once

#include "pch.h"
#include "Scenario1.g.h"
#include "MainPage.xaml.h"

namespace SDKSample
{
    namespace AccelerometerCPP
    {
        /// <summary>
        /// An empty page that can be used on its own or navigated to within a Frame.
        /// </summary>
    	[Windows::Foundation::Metadata::WebHostHidden]
        public ref class Scenario1 sealed
        {
        public:
            Scenario1();
    
        protected:
            virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
    
        private:
            typedef rxrt::EventPattern<Object^, Windows::UI::Xaml::RoutedEventArgs^> RoutedEventPattern;

            MainPage^ rootPage;
            Windows::Devices::Sensors::Accelerometer^ accelerometer;
            std::shared_ptr<rx::BehaviorSubject<bool>> enabled;
            rxrt::ReactiveCommand<RoutedEventPattern>::shared enable;
            rxrt::ReactiveCommand<RoutedEventPattern>::shared disable;
            rx::Disposable readingSubscription;
			rx::Disposable visibilitySubscription;
            uint32 desiredReportInterval;
        };
    }
}

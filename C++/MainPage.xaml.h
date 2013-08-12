//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
//
//*********************************************************

//
// MainPage.xaml.h
// Declaration of the MainPage.xaml class.
//

#pragma once

#include "pch.h"
#include "MainPage.g.h"
#include "Common\LayoutAwarePage.h" // Required by generated header
#include "Constants.h"

namespace SDKSample
{
    public enum class NotifyType
    {
        StatusMessage,
        ErrorMessage
    };

    public ref class MainPageSizeChangedEventArgs sealed
    {
    public:
        property double Width
        {
            double get()
            {
                return width;
            }

            void set(double value)
            {
                width = value;
            }
        }

    private:
        double width;
    };

    public ref class MainPage sealed
    {
    public:
		virtual ~MainPage();
		MainPage();

    protected:
        virtual void LoadState(Platform::Object^ navigationParameter,
            Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;
        virtual void SaveState(Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;

    internal:

        void NotifyUser(Platform::String^ strMessage, NotifyType type);
        void LoadScenario(Platform::String^ scenarioName);
        event Windows::Foundation::EventHandler<Platform::Object^>^ ScenarioLoaded;
        event Windows::Foundation::EventHandler<MainPageSizeChangedEventArgs^>^ MainPageResized;

    private:
        void PopulateScenarios();
        void InvalidateSize();
        void InvalidateLayout();

		rx::ComposableDisposable cd;

        Platform::Collections::Vector<Object^>^ ScenarioList;
        Windows::UI::Xaml::Controls::Frame^ HiddenFrame;
        void Footer_Click(Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

    internal:
        static MainPage^ Current;

    };
}

//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
//
//*********************************************************

//
// App.xaml.h
// Declaration of the App.xaml class.
//

#pragma once

#include "pch.h"
#include "App.g.h"
#include "MainPage.g.h"

namespace SDKSample
{
    ref class App
    {
    internal:
        App();
    protected:
        virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ pArgs) override;
    };
}

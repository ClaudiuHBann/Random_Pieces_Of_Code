#include <winrt/windows.foundation.h>

using namespace winrt;
using namespace Windows;
using namespace Foundation;

#include <chrono>
#include <iostream>

using namespace std;
using namespace chrono;

// async method
IAsyncOperation<int> Initialize()
{
    co_await resume_background(); // run in a threadpool and return to the caller

    // working...
    cout << "Initializing..." << endl;
    this_thread::sleep_for(3s);
    cout << "Done initializing..." << endl;

    // done
    co_return -69;
}

// async method
IAsyncOperation<bool> HasActiveLicense()
{
    co_await resume_background(); // run in a threadpool and return to the caller

    // working...
    cout << "Gathering license..." << endl;
    this_thread::sleep_for(2s);
    cout << "Invalid license..." << endl;

    // done
    co_return false;
}

IAsyncAction Run()
{
    // wait both of them
    /*
    auto initialized = co_await Initialize();
    auto isLicenseValid = co_await HasActiveLicense();
    */

    // GetLicense does not depend on Initializing so run initialize in background and verify license after which verify
    // if it was initialized successfully
    /*
     */
    auto operationInitialize = Initialize();
    auto isLicenseValid = co_await HasActiveLicense();
    auto initialized = co_await operationInitialize;

    cout << (initialized == -69 ? "Initialized" : "Failed initializing...") << endl;
    cout << (isLicenseValid ? "Valid license" : "Invalid license") << endl;

    co_return;
}

int main()
{
    Run().get();

    return 0;
}

// if the next operation depends on the last data continously in a method then co_await it's just fine

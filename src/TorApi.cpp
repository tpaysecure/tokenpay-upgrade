/**
 * Copyright (c) 2019 TokenPay
 *
 * Distributed under the MIT/X11 software license,
 * see the accompanying file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <TorApi.h>
#include <tor/src/feature/api/tor_tokenpay_api.h>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TorApi::StartDaemon(int iArgc, char* iArgv[])
{
    return TorTokenpayApi_StartDaemon(iArgc, iArgv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TorApi::IsMainLoopReady()
{
    return static_cast<bool>(TorTokenpayApi_IsMainLoopReady());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TorApi::HasAnyErrorOccurred()
{
    return static_cast<bool>(TorTokenpayApi_HasAnyErrorOccurred());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TorApi::HasShutdownBeenRequested()
{
    return static_cast<bool>(TorTokenpayApi_HasShutdownBeenRequested());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TorApi::IsBootstrapReady()
{
    bool isBootstrapReady = TorTokenpayApi_IsBootstrapReady();

    if (isBootstrapReady)
    {
        assert(IsMainLoopReady());
    }

    return isBootstrapReady;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TorApi::WaitOnConditionVariable()
{
    // if both steps are already complete or there's been errors, then it's an error for any thread
    // to wait on the Tor-synchronization-condition-variable because that CV would never be notified
    //
    assert(!HasShutdownBeenRequested() && !HasAnyErrorOccurred() && (!IsMainLoopReady() || !IsBootstrapReady()));

    TorTokenpayApi_WaitOnConditionVariable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TorApi::StopDaemon()
{
    assert(DaemonSynchronizationMgr::IsInstantiated());

    TorTokenpayApi_StopDaemon();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SingletonT>
TorApi::detail::EphemeralSingletonContainer<SingletonT>::~EphemeralSingletonContainer()
{
    assert(Self::IsInstantiated());

    r_isInstantiated.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SingletonT>
bool TorApi::detail::EphemeralSingletonContainer<SingletonT>::IsInstantiated()
{
    const auto C_OLD_VALUE = r_isInstantiated.test_and_set();
    if (!C_OLD_VALUE)
    {
        r_isInstantiated.clear();
    }

    return C_OLD_VALUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SingletonT>
TorApi::detail::EphemeralSingletonContainer<SingletonT>::EphemeralSingletonContainer()
{
    assert(!Self::IsInstantiated());

    r_isInstantiated.test_and_set();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SingletonT>
std::atomic_flag TorApi::detail::EphemeralSingletonContainer<SingletonT>::r_isInstantiated = ATOMIC_FLAG_INIT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TorApi::DaemonSynchronizationMgr::DaemonSynchronizationMgr()
: Super()
{
    assert(Super::IsInstantiated());

    TorTokenpayApi_InitializeSyncPrimitives();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TorApi::DaemonSynchronizationMgr::~DaemonSynchronizationMgr()
{
    assert(Super::IsInstantiated());

    TorTokenpayApi_CleanUpSyncPrimitives();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TorApi::DaemonSynchronizationMgr::LockGuard::LockGuard()
: Super(),
  r_isAcquired{false}
{
    assert(Super::IsInstantiated());
    assert(Outer::IsInstantiated());

    Self::acquire();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TorApi::DaemonSynchronizationMgr::LockGuard::~LockGuard()
{
    assert(Super::IsInstantiated());
    assert(Outer::IsInstantiated());

    if (Self::isAcquired())
    {
        Self::release();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TorApi::DaemonSynchronizationMgr::LockGuard::acquire()
{
    assert(!Self::isAcquired());

    r_isAcquired = true;
    TorTokenpayApi_AcquireMutex();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TorApi::DaemonSynchronizationMgr::LockGuard::release()
{
    assert(Self::isAcquired());

    r_isAcquired = false;
    TorTokenpayApi_ReleaseMutex();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool TorApi::DaemonSynchronizationMgr::LockGuard::isAcquired() const
{
    return r_isAcquired;
}

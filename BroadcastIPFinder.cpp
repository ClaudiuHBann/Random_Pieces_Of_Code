#define NOMINMAX

#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include <iostream>
#include <vector>

using namespace std;

class BroadcastIPFinder
{
  public:
    BroadcastIPFinder() : mBroadcastIPs(_GetBroadcastAddressesIPv4())
    {
        // we are adding the 255.255.255.255 broadcast address manually
        mBroadcastIPs.push_back(numeric_limits<uint32_t>::max());
    }

    const vector<uint32_t> &GetBroadcastAddressesIPv4() const
    {
        return mBroadcastIPs;
    }

    const vector<uint32_t> &GetLocalIPv4s() const
    {
        return mLocalIPs;
    }

  private:
    PIP_ADAPTER_ADDRESSES GetAdaptersAddressesIPv4()
    {
        ULONG ipAdapterAddressesSize = 8 * 1024;
        ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST |
                      GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_FRIENDLY_NAME;

        PIP_ADAPTER_ADDRESSES ipAdapterAddresses{};
        DWORD errorCode{};
        do
        {
            ipAdapterAddresses = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, ipAdapterAddressesSize);
            if (!ipAdapterAddresses)
            {
                wcout << L"Could not allocate memory IP adapter addresses because: " << GetLastError();
                return {};
            }

            // on ERROR_BUFFER_OVERFLOW ipAdapterAddressesSize will contain the nedded buffer size
            errorCode = GetAdaptersAddresses(AF_INET, flags, nullptr, ipAdapterAddresses, &ipAdapterAddressesSize);
            if (errorCode)
            {
                wcout << L"Could not get IP adapter addresses because: " << GetLastError();
                HeapFree(GetProcessHeap(), 0, ipAdapterAddresses);
                ipAdapterAddresses = {};
            }
        } while (errorCode == ERROR_BUFFER_OVERFLOW);

        return ipAdapterAddresses;
    }

    constexpr auto MakeBroadcastAddressIPv4(const uint32_t aAddress, const uint32_t aMask)
    {
        return aAddress | ~aMask;
    }

    vector<uint32_t> _GetBroadcastAddressesIPv4()
    {
        vector<uint32_t> broadcastAddrs{};

        for (const auto &[ipv4, mask] : GetNICsIPv4AndMask())
        {
            mLocalIPs.push_back(ipv4);
            broadcastAddrs.push_back(MakeBroadcastAddressIPv4(ipv4, mask));
        }

        return broadcastAddrs;
    }

    vector<pair<uint32_t, uint32_t>> GetNICsIPv4AndMask()
    {
        auto adaptersAddressesIPv4 = GetAdaptersAddressesIPv4();
        if (!adaptersAddressesIPv4)
        {
            return {};
        }

        vector<pair<uint32_t, uint32_t>> nicsIPv4AndMask{};
        for (auto adaptersAddressesIPv4Current = adaptersAddressesIPv4; adaptersAddressesIPv4Current;
             adaptersAddressesIPv4Current = adaptersAddressesIPv4Current->Next)
        {
            if (adaptersAddressesIPv4Current->OperStatus != IfOperStatusUp)
            {
                continue;
            }

            for (auto unicastAddressCurrent = adaptersAddressesIPv4Current->FirstUnicastAddress; unicastAddressCurrent;
                 unicastAddressCurrent = unicastAddressCurrent->Next)
            {
                if (!unicastAddressCurrent->Address.lpSockaddr)
                {
                    continue;
                }

                if (unicastAddressCurrent->Address.lpSockaddr->sa_family == AF_INET)
                {
                    auto mask = numeric_limits<uint32_t>::max();
                    if (unicastAddressCurrent->OnLinkPrefixLength <= 32)
                    {
                        mask <<= 32 - unicastAddressCurrent->OnLinkPrefixLength;
                        mask = htonl(mask);
                    }

                    auto addr = (sockaddr_in *)unicastAddressCurrent->Address.lpSockaddr;
                    nicsIPv4AndMask.emplace_back(addr->sin_addr.S_un.S_addr, mask);
                }
            }
        }

        HeapFree(GetProcessHeap(), 0, adaptersAddressesIPv4);
        return nicsIPv4AndMask;
    }

    vector<uint32_t> mLocalIPs;
    vector<uint32_t> mBroadcastIPs;
};

int main()
{
    BroadcastIPFinder broadcastIPFinder;

    wcout << L"Local IPs:" << endl << endl;
    for (const auto address : broadcastIPFinder.GetLocalIPv4s())
    {
        in_addr addr{};
        addr.S_un.S_addr = address;

        wchar_t ip[INET6_ADDRSTRLEN]{};
        InetNtopW(AF_INET, &addr, ip, INET6_ADDRSTRLEN);
        wcout << ip << endl;
    }

    wcout << endl << L"Broadcast IPs:" << endl << endl;
    for (const auto address : broadcastIPFinder.GetBroadcastAddressesIPv4())
    {
        in_addr addr{};
        addr.S_un.S_addr = address;

        wchar_t ip[INET6_ADDRSTRLEN]{};
        InetNtopW(AF_INET, &addr, ip, INET6_ADDRSTRLEN);
        wcout << ip << endl;
    }

    return 0;
}

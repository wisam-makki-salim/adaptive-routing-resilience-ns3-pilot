#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/wifi-module.h"

#include <iostream>
#include <vector>

using namespace ns3;

static std::vector<double> g_helloTimes;

static void
ObserveOlsrTx(const olsr::PacketHeader&, const olsr::MessageList& messages)
{
    for (const auto& message : messages)
    {
        if (message.GetMessageType() == olsr::MessageHeader::HELLO_MESSAGE)
        {
            g_helloTimes.push_back(Simulator::Now().GetSeconds());
        }
    }
}

static void
SetFastHello(Ptr<olsr::RoutingProtocol> protocol)
{
    protocol->SetAttribute("HelloInterval", TimeValue(Seconds(0.5)));
}

int
main()
{
    RngSeedManager::SetSeed(20260920);
    RngSeedManager::SetRun(1);

    NodeContainer nodes;
    nodes.Create(2);
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    OlsrHelper olsr;
    Ipv4ListRoutingHelper routing;
    routing.Add(olsr, 10);
    InternetStackHelper internet;
    internet.SetRoutingHelper(routing);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.2.0.0", "255.255.255.0");
    ipv4.Assign(devices);

    Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting>(nodes.Get(0)->GetObject<Ipv4>()->GetRoutingProtocol());
    int16_t priority = 0;
    Ptr<olsr::RoutingProtocol> protocol = DynamicCast<olsr::RoutingProtocol>(list->GetRoutingProtocol(0, priority));
    if (!protocol)
    {
        std::cerr << "Could not retrieve OLSR protocol" << std::endl;
        return 2;
    }
    protocol->TraceConnectWithoutContext("Tx", MakeCallback(&ObserveOlsrTx));
    Simulator::Schedule(Seconds(6.0), &SetFastHello, protocol);
    Simulator::Stop(Seconds(11.0));
    Simulator::Run();

    uint32_t fastIntervals = 0;
    for (std::size_t i = 1; i < g_helloTimes.size(); ++i)
    {
        const double delta = g_helloTimes[i] - g_helloTimes[i - 1];
        if (g_helloTimes[i - 1] >= 6.0 && delta < 0.75)
        {
            ++fastIntervals;
        }
    }

    std::cout << "G1_OLSR_RUNTIME hello_count=" << g_helloTimes.size()
              << " post_change_fast_intervals=" << fastIntervals << " times=";
    for (double time : g_helloTimes)
    {
        std::cout << time << ",";
    }
    std::cout << std::endl;

    Simulator::Destroy();
    return fastIntervals >= 3 ? 0 : 3;
}

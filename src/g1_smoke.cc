#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/wifi-module.h"

#include <iomanip>
#include <iostream>

using namespace ns3;

int
main(int argc, char* argv[])
{
    uint32_t run = 1;
    CommandLine cmd(__FILE__);
    cmd.AddValue("run", "Independent ns-3 run number", run);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(20260920);
    RngSeedManager::SetRun(run);

    NodeContainer nodes;
    nodes.Create(3);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positions = CreateObject<ListPositionAllocator>();
    positions->Add(Vector(0.0, 0.0, 0.0));
    positions->Add(Vector(60.0, 0.0, 0.0));
    positions->Add(Vector(120.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positions);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(75.0));

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("ErpOfdmRate6Mbps"),
                                 "ControlMode",
                                 StringValue("ErpOfdmRate6Mbps"));

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
    ipv4.SetBase("10.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    constexpr uint16_t port = 9000;
    UdpServerHelper server(port);
    ApplicationContainer serverApp = server.Install(nodes.Get(2));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(25.0));

    UdpClientHelper client(interfaces.GetAddress(2), port);
    client.SetAttribute("MaxPackets", UintegerValue(100));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
    client.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(10.0));
    clientApp.Stop(Seconds(20.5));

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(25.0));
    Simulator::Run();
    monitor->CheckForLostPackets();

    uint64_t txPackets = 0;
    uint64_t rxPackets = 0;
    Time delaySum = Seconds(0);
    for (const auto& [flowId, stats] : monitor->GetFlowStats())
    {
        txPackets += stats.txPackets;
        rxPackets += stats.rxPackets;
        delaySum += stats.delaySum;
    }

    const double pdr = txPackets ? 100.0 * rxPackets / txPackets : 0.0;
    const double meanDelayMs = rxPackets ? 1000.0 * delaySum.GetSeconds() / rxPackets : 0.0;
    std::cout << std::fixed << std::setprecision(3)
              << "G1_SMOKE ns3=3.47 run=" << run << " nodes=3 hops=2 tx=" << txPackets
              << " rx=" << rxPackets << " pdr_percent=" << pdr
              << " mean_delay_ms=" << meanDelayMs << std::endl;

    Simulator::Destroy();
    return rxPackets > 0 ? 0 : 2;
}

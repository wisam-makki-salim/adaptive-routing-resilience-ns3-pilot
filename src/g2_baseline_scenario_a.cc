#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/wifi-module.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace ns3;

static uint64_t g_controlPackets = 0;
static uint64_t g_controlBytes = 0;
static uint64_t g_nextHopChanges = 0;
static uint64_t g_routeUnavailableSamples = 0;
static std::vector<Ptr<olsr::RoutingProtocol>> g_sourceProtocols;
static std::vector<Ipv4Address> g_flowDestinations;
static std::vector<Ipv4Address> g_lastNextHops;
static std::vector<bool> g_hadRoute;

static void
CountOlsrTx(const olsr::PacketHeader& header, const olsr::MessageList&)
{
    if (Simulator::Now() < Seconds(20.0) || Simulator::Now() > Seconds(110.0))
    {
        return;
    }
    ++g_controlPackets;
    g_controlBytes += header.GetPacketLength();
}

static void
SampleFlowRoutes()
{
    for (std::size_t flow = 0; flow < g_sourceProtocols.size(); ++flow)
    {
        bool found = false;
        for (const auto& entry : g_sourceProtocols[flow]->GetRoutingTableEntries())
        {
            if (entry.destAddr != g_flowDestinations[flow])
            {
                continue;
            }
            found = true;
            if (g_hadRoute[flow] && entry.nextAddr != g_lastNextHops[flow])
            {
                ++g_nextHopChanges;
            }
            g_lastNextHops[flow] = entry.nextAddr;
            g_hadRoute[flow] = true;
            break;
        }
        if (!found)
        {
            ++g_routeUnavailableSamples;
            g_hadRoute[flow] = false;
        }
    }
    if (Simulator::Now() + MilliSeconds(250) <= Seconds(120.0))
    {
        Simulator::Schedule(MilliSeconds(250), &SampleFlowRoutes);
    }
}

static Ptr<olsr::RoutingProtocol>
GetOlsr(Ptr<Node> node)
{
    Ptr<Ipv4ListRouting> list =
        DynamicCast<Ipv4ListRouting>(node->GetObject<Ipv4>()->GetRoutingProtocol());
    if (!list)
    {
        return nullptr;
    }
    for (uint32_t i = 0; i < list->GetNRoutingProtocols(); ++i)
    {
        int16_t priority = 0;
        Ptr<olsr::RoutingProtocol> protocol =
            DynamicCast<olsr::RoutingProtocol>(list->GetRoutingProtocol(i, priority));
        if (protocol)
        {
            return protocol;
        }
    }
    return nullptr;
}

int
main(int argc, char* argv[])
{
    uint32_t run = 1;
    CommandLine cmd(__FILE__);
    cmd.AddValue("run", "Independent development run number", run);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(20260920);
    RngSeedManager::SetRun(run);

    constexpr uint32_t nodeCount = 16;
    constexpr double spacing = 60.0;
    constexpr double maxRange = 75.0;
    constexpr double appStop = 110.0;
    constexpr double simulationStop = 120.0;
    constexpr uint32_t packetSize = 512;
    constexpr double packetInterval = 0.02;

    NodeContainer nodes;
    nodes.Create(nodeCount);

    Ptr<UniformRandomVariable> positionJitter = CreateObject<UniformRandomVariable>();
    positionJitter->SetAttribute("Min", DoubleValue(-2.0));
    positionJitter->SetAttribute("Max", DoubleValue(2.0));
    positionJitter->SetStream(10);
    Ptr<ListPositionAllocator> positions = CreateObject<ListPositionAllocator>();
    for (uint32_t row = 0; row < 4; ++row)
    {
        for (uint32_t col = 0; col < 4; ++col)
        {
            positions->Add(Vector(col * spacing + positionJitter->GetValue(),
                                  row * spacing + positionJitter->GetValue(),
                                  0.0));
        }
    }
    MobilityHelper mobility;
    mobility.SetPositionAllocator(positions);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::RangePropagationLossModel",
                               "MaxRange",
                               DoubleValue(maxRange));
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
    olsr.Set("HelloInterval", TimeValue(Seconds(2.0)));
    olsr.Set("TcInterval", TimeValue(Seconds(5.0)));
    Ipv4ListRoutingHelper routing;
    routing.Add(olsr, 10);
    InternetStackHelper internet;
    internet.SetRoutingHelper(routing);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.10.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    for (uint32_t i = 0; i < nodeCount; ++i)
    {
        Ptr<olsr::RoutingProtocol> protocol = GetOlsr(nodes.Get(i));
        if (!protocol)
        {
            std::cerr << "Unable to retrieve OLSR on node " << i << std::endl;
            return 2;
        }
        protocol->TraceConnectWithoutContext("Tx", MakeCallback(&CountOlsrTx));
    }

    const std::vector<uint32_t> sources{0, 3};
    const std::vector<uint32_t> destinations{15, 12};
    const std::vector<uint16_t> ports{9000, 9001};
    Ptr<UniformRandomVariable> startJitter = CreateObject<UniformRandomVariable>();
    startJitter->SetAttribute("Min", DoubleValue(0.0));
    startJitter->SetAttribute("Max", DoubleValue(0.5));
    startJitter->SetStream(20);

    for (uint32_t flow = 0; flow < sources.size(); ++flow)
    {
        UdpServerHelper server(ports[flow]);
        ApplicationContainer serverApp = server.Install(nodes.Get(destinations[flow]));
        serverApp.Start(Seconds(0.0));
        serverApp.Stop(Seconds(simulationStop));

        UdpClientHelper client(interfaces.GetAddress(destinations[flow]), ports[flow]);
        client.SetAttribute("MaxPackets", UintegerValue(1000000));
        client.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));
        ApplicationContainer clientApp = client.Install(nodes.Get(sources[flow]));
        clientApp.Start(Seconds(20.0 + startJitter->GetValue()));
        clientApp.Stop(Seconds(appStop));

        g_sourceProtocols.push_back(GetOlsr(nodes.Get(sources[flow])));
        g_flowDestinations.push_back(interfaces.GetAddress(destinations[flow]));
        g_lastNextHops.emplace_back();
        g_hadRoute.push_back(false);
    }

    Simulator::Schedule(Seconds(20.0), &SampleFlowRoutes);

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();
    Simulator::Stop(Seconds(simulationStop));
    Simulator::Run();
    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    uint64_t txPackets = 0;
    uint64_t rxPackets = 0;
    Time delaySum = Seconds(0.0);
    std::vector<double> perFlowPdr;
    std::vector<uint64_t> delayBins;
    double delayBinWidth = 0.001;
    for (const auto& [flowId, stats] : monitor->GetFlowStats())
    {
        const auto tuple = classifier->FindFlow(flowId);
        if (tuple.protocol != 17 ||
            std::find(ports.begin(), ports.end(), tuple.destinationPort) == ports.end())
        {
            continue;
        }
        txPackets += stats.txPackets;
        rxPackets += stats.rxPackets;
        delaySum += stats.delaySum;
        perFlowPdr.push_back(stats.txPackets ? 100.0 * stats.rxPackets / stats.txPackets : 0.0);
        if (stats.delayHistogram.GetNBins() > 0)
        {
            delayBinWidth = stats.delayHistogram.GetBinEnd(0) -
                            stats.delayHistogram.GetBinStart(0);
        }
        delayBins.resize(std::max<std::size_t>(delayBins.size(),
                                               stats.delayHistogram.GetNBins()),
                         0);
        for (uint32_t bin = 0; bin < stats.delayHistogram.GetNBins(); ++bin)
        {
            delayBins[bin] += stats.delayHistogram.GetBinCount(bin);
        }
    }

    auto histogramQuantileMs = [&](double quantile) {
        const uint64_t target = static_cast<uint64_t>(std::ceil(quantile * rxPackets));
        uint64_t cumulative = 0;
        for (std::size_t bin = 0; bin < delayBins.size(); ++bin)
        {
            cumulative += delayBins[bin];
            if (cumulative >= target)
            {
                return 1000.0 * (bin + 1) * delayBinWidth;
            }
        }
        return 0.0;
    };

    const uint64_t lostPackets = txPackets - rxPackets;
    const double pdr = txPackets ? 100.0 * rxPackets / txPackets : 0.0;
    const double meanDelayMs = rxPackets ? 1000.0 * delaySum.GetSeconds() / rxPackets : 0.0;
    const double delayP50Ms = histogramQuantileMs(0.50);
    const double delayP95Ms = histogramQuantileMs(0.95);
    const double offeredDuration = appStop - 20.0;
    const double goodputKbps = (rxPackets * packetSize * 8.0) / offeredDuration / 1000.0;
    const double normalizedOverhead =
        rxPackets ? static_cast<double>(g_controlBytes) / (rxPackets * packetSize) : 0.0;
    const double minFlowPdr = perFlowPdr.empty()
                                  ? 0.0
                                  : *std::min_element(perFlowPdr.begin(), perFlowPdr.end());

    std::cout << std::fixed << std::setprecision(6)
              << run << "," << txPackets << "," << rxPackets << "," << lostPackets << ","
              << pdr << "," << minFlowPdr << "," << meanDelayMs << "," << delayP50Ms
              << "," << delayP95Ms << "," << goodputKbps
              << "," << g_controlPackets << "," << g_controlBytes << ","
              << normalizedOverhead << "," << g_nextHopChanges << ","
              << g_routeUnavailableSamples << std::endl;

    Simulator::Destroy();
    return (perFlowPdr.size() == 2 && rxPackets > 0) ? 0 : 3;
}

#include "lra_olsr_controller.h"

#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/olsr-helper.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace ns3;

static uint64_t g_controlPackets = 0;
static uint64_t g_controlBytes = 0;
static uint64_t g_nextHopChanges = 0;
static uint64_t g_routeUnavailableSamples = 0;
static std::vector<uint64_t> g_rxBins(121, 0);
static std::vector<std::vector<uint64_t>> g_rxFlowBins(2, std::vector<uint64_t>(121, 0));
static std::vector<Ptr<olsr::RoutingProtocol>> g_protocols;
static std::vector<std::unique_ptr<LraOlsrController>> g_controllers;
static std::vector<Ipv4Address> g_flowDestinations;
static std::vector<Mac48Address> g_nodeMacs;
static std::map<Ipv4Address, Mac48Address> g_ipToMac;
static std::map<Ipv4Address, uint32_t> g_ipToNode;
static NetDeviceContainer g_devices;
static int32_t g_disruptedNode = -1;
static std::vector<Ipv4Address> g_lastSourceNextHops(2);
static std::vector<bool> g_hadSourceRoute(2, false);

struct RouteRecord
{
    double timeSeconds;
    uint32_t flow;
    std::string event;
    Ipv4Address nextHop;
};
static std::vector<RouteRecord> g_routeRecords;

static void
CountOlsrTx(const olsr::PacketHeader& header, const olsr::MessageList&)
{
    if (Simulator::Now() >= Seconds(20.0) && Simulator::Now() <= Seconds(110.0))
    {
        ++g_controlPackets;
        g_controlBytes += header.GetPacketLength();
    }
}

static void
CountApplicationRx(uint32_t flow, Ptr<const Packet>, const Address&, const Address&)
{
    const uint32_t second = std::min<uint32_t>(120, Simulator::Now().GetSeconds());
    ++g_rxBins[second];
    ++g_rxFlowBins[flow][second];
}

static Ptr<olsr::RoutingProtocol>
GetOlsr(Ptr<Node> node)
{
    Ptr<Ipv4ListRouting> list =
        DynamicCast<Ipv4ListRouting>(node->GetObject<Ipv4>()->GetRoutingProtocol());
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

static bool
FindRoute(Ptr<olsr::RoutingProtocol> protocol,
          Ipv4Address destination,
          olsr::RoutingTableEntry& result)
{
    for (const auto& entry : protocol->GetRoutingTableEntries())
    {
        if (entry.destAddr == destination)
        {
            result = entry;
            return true;
        }
    }
    return false;
}

static void
UpdateRouteState()
{
    if (!g_controllers.empty())
    {
        for (uint32_t node = 0; node < g_protocols.size(); ++node)
        {
            std::set<Mac48Address> monitored;
            for (Ipv4Address destination : g_flowDestinations)
            {
                olsr::RoutingTableEntry entry;
                if (FindRoute(g_protocols[node], destination, entry))
                {
                    auto mac = g_ipToMac.find(entry.nextAddr);
                    if (mac != g_ipToMac.end())
                    {
                        monitored.insert(mac->second);
                    }
                }
            }
            g_controllers[node]->SetMonitoredNeighbors(monitored);
        }
    }

    const std::vector<uint32_t> sources{0, 3};
    for (uint32_t flow = 0; flow < sources.size(); ++flow)
    {
        olsr::RoutingTableEntry entry;
        if (FindRoute(g_protocols[sources[flow]], g_flowDestinations[flow], entry))
        {
            if (g_hadSourceRoute[flow] && entry.nextAddr != g_lastSourceNextHops[flow])
            {
                ++g_nextHopChanges;
                g_routeRecords.push_back({Simulator::Now().GetSeconds(),
                                          flow,
                                          "next_hop_change",
                                          entry.nextAddr});
            }
            else if (!g_hadSourceRoute[flow])
            {
                g_routeRecords.push_back({Simulator::Now().GetSeconds(),
                                          flow,
                                          "route_available",
                                          entry.nextAddr});
            }
            g_lastSourceNextHops[flow] = entry.nextAddr;
            g_hadSourceRoute[flow] = true;
        }
        else
        {
            ++g_routeUnavailableSamples;
            if (g_hadSourceRoute[flow])
            {
                g_routeRecords.push_back({Simulator::Now().GetSeconds(),
                                          flow,
                                          "route_unavailable",
                                          Ipv4Address("0.0.0.0")});
            }
            g_hadSourceRoute[flow] = false;
        }
    }

    if (Simulator::Now() + MilliSeconds(250) <= Seconds(120.0))
    {
        Simulator::Schedule(MilliSeconds(250), &UpdateRouteState);
    }
}

static int32_t
GetActiveNextHopNode()
{
    olsr::RoutingTableEntry entry;
    if (!FindRoute(g_protocols[0], g_flowDestinations[0], entry))
    {
        return -1;
    }
    auto node = g_ipToNode.find(entry.nextAddr);
    return node == g_ipToNode.end() ? -1 : static_cast<int32_t>(node->second);
}

static void
StartMobilityDegradation()
{
    g_disruptedNode = GetActiveNextHopNode();
    if (g_disruptedNode < 0)
    {
        return;
    }
    Ptr<WaypointMobilityModel> model =
        NodeList::GetNode(g_disruptedNode)->GetObject<WaypointMobilityModel>();
    Vector current = model->GetPosition();
    Vector target(current.x * 2.5, current.y * 2.5, 0.0);
    if (std::abs(target.x) < 10.0)
    {
        target.x -= 90.0;
    }
    if (std::abs(target.y) < 10.0)
    {
        target.y -= 90.0;
    }
    model->AddWaypoint(Waypoint(Seconds(50.0), current));
    model->AddWaypoint(Waypoint(Seconds(75.0), target));
    model->AddWaypoint(Waypoint(Seconds(120.0), target));
}

static void
FailActiveNextHop()
{
    g_disruptedNode = GetActiveNextHopNode();
    if (g_disruptedNode < 0)
    {
        return;
    }
    Ptr<WifiNetDevice> device =
        DynamicCast<WifiNetDevice>(g_devices.Get(static_cast<uint32_t>(g_disruptedNode)));
    device->GetPhy()->SetOffMode();
}

static double
ComputeRecoverySeconds(const std::string& scenario)
{
    if (scenario == "A")
    {
        return -1.0;
    }
    const uint32_t eventSecond = scenario == "B" ? 75 : 60;
    const uint32_t preStart = scenario == "B" ? 40 : 50;
    std::vector<uint64_t> pre(g_rxBins.begin() + preStart,
                              g_rxBins.begin() + preStart + 10);
    std::sort(pre.begin(), pre.end());
    const double baseline = 0.5 * (pre[4] + pre[5]);
    const double threshold = 0.9 * baseline;
    for (uint32_t start = eventSecond; start + 4 < 110; ++start)
    {
        bool sustained = true;
        for (uint32_t second = start; second < start + 5; ++second)
        {
            sustained = sustained && g_rxBins[second] >= threshold;
        }
        if (sustained)
        {
            return start - eventSecond;
        }
    }
    return -2.0;
}

int
main(int argc, char* argv[])
{
    std::string scenario = "A";
    std::string mechanism = "baseline";
    std::string tracePrefix;
    double riskThresholdDbm = -81.0;
    uint32_t finalFailureThreshold = 40;
    bool enableSignalTrigger = true;
    bool enableFailureTrigger = true;
    double reactiveHelloSeconds = 0.5;
    double reactiveTcSeconds = 1.0;
    uint32_t run = 101;
    CommandLine cmd(__FILE__);
    cmd.AddValue("scenario", "A, B, or C", scenario);
    cmd.AddValue("mechanism", "baseline or adaptive", mechanism);
    cmd.AddValue("run", "Independent ns-3 run number", run);
    cmd.AddValue("tracePrefix", "Optional prefix for detailed G5 CSV traces", tracePrefix);
    cmd.AddValue("riskThresholdDbm", "Adaptive signal-risk threshold", riskThresholdDbm);
    cmd.AddValue("finalFailureThreshold", "Final Tx failures required to react", finalFailureThreshold);
    cmd.AddValue("enableSignalTrigger", "Enable signal-risk transitions", enableSignalTrigger);
    cmd.AddValue("enableFailureTrigger", "Enable failure transitions", enableFailureTrigger);
    cmd.AddValue("reactiveHelloSeconds", "Reactive OLSR HELLO interval", reactiveHelloSeconds);
    cmd.AddValue("reactiveTcSeconds", "Reactive OLSR TC interval", reactiveTcSeconds);
    cmd.Parse(argc, argv);
    if ((scenario != "A" && scenario != "B" && scenario != "C") ||
        (mechanism != "baseline" && mechanism != "adaptive"))
    {
        std::cerr << "Invalid scenario or mechanism" << std::endl;
        return 2;
    }

    RngSeedManager::SetSeed(20260920);
    RngSeedManager::SetRun(run);
    constexpr uint32_t nodeCount = 16;
    constexpr uint32_t packetSize = 512;

    NodeContainer nodes;
    nodes.Create(nodeCount);
    Ptr<UniformRandomVariable> positionJitter = CreateObject<UniformRandomVariable>();
    positionJitter->SetAttribute("Min", DoubleValue(-2.0));
    positionJitter->SetAttribute("Max", DoubleValue(2.0));
    positionJitter->SetStream(10);
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::WaypointMobilityModel");
    mobility.Install(nodes);
    for (uint32_t row = 0; row < 4; ++row)
    {
        for (uint32_t col = 0; col < 4; ++col)
        {
            const uint32_t node = row * 4 + col;
            Ptr<WaypointMobilityModel> model = nodes.Get(node)->GetObject<WaypointMobilityModel>();
            Vector initial(col * 60.0 + positionJitter->GetValue(),
                           row * 60.0 + positionJitter->GetValue(),
                           0.0);
            model->AddWaypoint(Waypoint(Seconds(0.0), initial));
            model->AddWaypoint(Waypoint(Seconds(49.999), initial));
        }
    }

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                               "Exponent",
                               DoubleValue(2.5),
                               "ReferenceDistance",
                               DoubleValue(1.0),
                               "ReferenceLoss",
                               DoubleValue(46.6777));
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    phy.Set("TxPowerStart", DoubleValue(20.0));
    phy.Set("TxPowerEnd", DoubleValue(20.0));
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
    g_devices = devices;

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

    for (uint32_t node = 0; node < nodeCount; ++node)
    {
        g_protocols.push_back(GetOlsr(nodes.Get(node)));
        g_protocols.back()->TraceConnectWithoutContext("Tx", MakeCallback(&CountOlsrTx));
        Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice>(devices.Get(node));
        Mac48Address nodeMac = Mac48Address::ConvertFrom(device->GetAddress());
        g_nodeMacs.push_back(nodeMac);
        g_ipToMac[interfaces.GetAddress(node)] = nodeMac;
        g_ipToNode[interfaces.GetAddress(node)] = node;
    }

    const std::vector<uint32_t> sources{0, 3};
    const std::vector<uint32_t> destinations{15, 12};
    const std::vector<uint16_t> ports{9000, 9001};
    g_flowDestinations = {interfaces.GetAddress(15), interfaces.GetAddress(12)};

    if (mechanism == "adaptive")
    {
        for (uint32_t node = 0; node < nodeCount; ++node)
        {
            g_controllers.push_back(std::make_unique<LraOlsrController>(
                g_protocols[node],
                riskThresholdDbm,
                -75.0,
                0.3,
                3,
                finalFailureThreshold,
                Seconds(10.0),
                true,
                enableSignalTrigger,
                enableFailureTrigger,
                Seconds(reactiveHelloSeconds),
                Seconds(reactiveTcSeconds)));
            g_controllers.back()->SetMonitoredNeighbors({});
            Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice>(devices.Get(node));
            device->GetPhy()->TraceConnectWithoutContext(
                "MonitorSnifferRx",
                MakeCallback(&LraOlsrController::ObserveMonitorSnifferRx,
                             g_controllers.back().get()));
            device->GetRemoteStationManager()->TraceConnectWithoutContext(
                "MacTxFinalDataFailed",
                MakeCallback(&LraOlsrController::ObserveFinalTxFailure,
                             g_controllers.back().get()));
        }
    }

    Ptr<UniformRandomVariable> startJitter = CreateObject<UniformRandomVariable>();
    startJitter->SetAttribute("Min", DoubleValue(0.0));
    startJitter->SetAttribute("Max", DoubleValue(0.5));
    startJitter->SetStream(20);
    for (uint32_t flow = 0; flow < sources.size(); ++flow)
    {
        UdpServerHelper server(ports[flow]);
        ApplicationContainer serverApp = server.Install(nodes.Get(destinations[flow]));
        serverApp.Start(Seconds(0.0));
        serverApp.Stop(Seconds(120.0));
        serverApp.Get(0)->TraceConnectWithoutContext(
            "RxWithAddresses",
            MakeBoundCallback(&CountApplicationRx, flow));

        UdpClientHelper client(interfaces.GetAddress(destinations[flow]), ports[flow]);
        client.SetAttribute("MaxPackets", UintegerValue(1000000));
        client.SetAttribute("Interval", TimeValue(Seconds(0.02)));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));
        ApplicationContainer clientApp = client.Install(nodes.Get(sources[flow]));
        clientApp.Start(Seconds(20.0 + startJitter->GetValue()));
        clientApp.Stop(Seconds(110.0));
    }

    Simulator::Schedule(Seconds(20.0), &UpdateRouteState);
    if (scenario == "B")
    {
        Simulator::Schedule(Seconds(50.0), &StartMobilityDegradation);
    }
    if (scenario == "C")
    {
        Simulator::Schedule(Seconds(60.0), &FailActiveNextHop);
    }

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();
    Simulator::Stop(Seconds(120.0));
    Simulator::Run();
    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    uint64_t txPackets = 0;
    uint64_t rxPackets = 0;
    Time delaySum = Seconds(0.0);
    std::vector<double> perFlowPdr;
    std::vector<uint64_t> delayBins;
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
        delayBins.resize(std::max<std::size_t>(delayBins.size(),
                                               stats.delayHistogram.GetNBins()),
                         0);
        for (uint32_t bin = 0; bin < stats.delayHistogram.GetNBins(); ++bin)
        {
            delayBins[bin] += stats.delayHistogram.GetBinCount(bin);
        }
    }
    auto quantileMs = [&](double q) {
        uint64_t target = static_cast<uint64_t>(std::ceil(q * rxPackets));
        uint64_t cumulative = 0;
        for (uint32_t bin = 0; bin < delayBins.size(); ++bin)
        {
            cumulative += delayBins[bin];
            if (cumulative >= target)
            {
                return static_cast<double>(bin + 1);
            }
        }
        return 0.0;
    };

    uint64_t signalTransitions = 0;
    uint64_t failureTransitions = 0;
    uint64_t recoveries = 0;
    for (const auto& controller : g_controllers)
    {
        for (const auto& transition : controller->GetTransitions())
        {
            signalTransitions += transition.reason == "signal_risk";
            failureTransitions += transition.reason == "final_tx_failures";
            recoveries += transition.reason == "recovery_hold_down";
        }
    }

    const double pdr = txPackets ? 100.0 * rxPackets / txPackets : 0.0;
    const double minFlowPdr = perFlowPdr.empty()
                                  ? 0.0
                                  : *std::min_element(perFlowPdr.begin(), perFlowPdr.end());
    const double meanDelayMs = rxPackets ? 1000.0 * delaySum.GetSeconds() / rxPackets : 0.0;
    const double goodputKbps = rxPackets * packetSize * 8.0 / 90.0 / 1000.0;
    const double normalizedOverhead =
        rxPackets ? static_cast<double>(g_controlBytes) / (rxPackets * packetSize) : 0.0;
    const double recoverySeconds = ComputeRecoverySeconds(scenario);

    if (!tracePrefix.empty())
    {
        std::ofstream rxTrace(tracePrefix + "_rx.csv");
        rxTrace << "second,rx_packets,flow_0_rx_packets,flow_1_rx_packets\n";
        for (uint32_t second = 0; second < g_rxBins.size(); ++second)
        {
            rxTrace << second << "," << g_rxBins[second] << ","
                    << g_rxFlowBins[0][second] << "," << g_rxFlowBins[1][second] << "\n";
        }

        std::ofstream routeTrace(tracePrefix + "_routes.csv");
        routeTrace << "time_seconds,flow,event,next_hop,next_hop_node\n";
        for (const auto& record : g_routeRecords)
        {
            auto node = g_ipToNode.find(record.nextHop);
            const int32_t nextHopNode =
                node == g_ipToNode.end() ? -1 : static_cast<int32_t>(node->second);
            routeTrace << std::fixed << std::setprecision(6) << record.timeSeconds << ","
                       << record.flow << "," << record.event << "," << record.nextHop << ","
                       << nextHopNode << "\n";
        }

        std::ofstream transitionTrace(tracePrefix + "_transitions.csv");
        transitionTrace << "node,time_seconds,state,reason,neighbor\n";
        for (uint32_t node = 0; node < g_controllers.size(); ++node)
        {
            for (const auto& transition : g_controllers[node]->GetTransitions())
            {
                transitionTrace << node << "," << std::fixed << std::setprecision(6)
                                << transition.timeSeconds << ","
                                << (transition.state == LraOlsrController::State::STABLE
                                        ? "stable"
                                        : "reactive")
                                << "," << transition.reason << "," << transition.neighbor
                                << "\n";
            }
        }
    }

    std::cout << std::fixed << std::setprecision(6) << scenario << "," << mechanism << ","
              << run << "," << txPackets << "," << rxPackets << "," << txPackets - rxPackets
              << "," << pdr << "," << minFlowPdr << "," << meanDelayMs << ","
              << quantileMs(0.50) << "," << quantileMs(0.95) << "," << goodputKbps << ","
              << g_controlPackets << "," << g_controlBytes << "," << normalizedOverhead << ","
              << recoverySeconds << "," << g_nextHopChanges << ","
              << g_routeUnavailableSamples << "," << signalTransitions << ","
              << failureTransitions << "," << recoveries << "," << g_disruptedNode << std::endl;

    Simulator::Destroy();
    return perFlowPdr.size() == 2 ? 0 : 3;
}

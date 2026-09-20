#include "lra_olsr_controller.h"

#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/olsr-helper.h"

#include <iostream>

using namespace ns3;

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
TimerEquals(Ptr<olsr::RoutingProtocol> protocol, const std::string& attribute, double seconds)
{
    TimeValue value;
    protocol->GetAttribute(attribute, value);
    return std::abs(value.Get().GetSeconds() - seconds) < 1e-9;
}

int
main()
{
    RngSeedManager::SetSeed(20260920);
    RngSeedManager::SetRun(1);

    NodeContainer nodes;
    nodes.Create(2);
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positions = CreateObject<ListPositionAllocator>();
    positions->Add(Vector(0.0, 0.0, 0.0));
    positions->Add(Vector(10.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positions);
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
    ipv4.SetBase("10.3.0.0", "255.255.255.0");
    ipv4.Assign(devices);

    Ptr<olsr::RoutingProtocol> realTraceProtocol = GetOlsr(nodes.Get(0));
    Ptr<olsr::RoutingProtocol> deterministicProtocol = GetOlsr(nodes.Get(1));
    LraOlsrController realTraceController(realTraceProtocol,
                                          100.0,
                                          101.0,
                                          0.3,
                                          3,
                                          40,
                                          Seconds(10.0),
                                          true);
    LraOlsrController deterministicController(deterministicProtocol);

    Ptr<WifiNetDevice> wifi0 = DynamicCast<WifiNetDevice>(devices.Get(0));
    wifi0->GetPhy()->TraceConnectWithoutContext(
        "MonitorSnifferRx",
        MakeCallback(&LraOlsrController::ObserveMonitorSnifferRx, &realTraceController));
    wifi0->GetRemoteStationManager()->TraceConnectWithoutContext(
        "MacTxFinalDataFailed",
        MakeCallback(&LraOlsrController::ObserveFinalTxFailure, &realTraceController));

    Simulator::Schedule(Seconds(1.0), &LraOlsrController::ObserveSignalDbm,
                        &deterministicController, -65.0);
    for (uint32_t i = 0; i < 6; ++i)
    {
        Simulator::Schedule(Seconds(3.0 + 0.1 * i),
                            &LraOlsrController::ObserveSignalDbm,
                            &deterministicController,
                            -90.0);
    }
    for (uint32_t i = 0; i < 8; ++i)
    {
        Simulator::Schedule(Seconds(5.0 + 0.1 * i),
                            &LraOlsrController::ObserveSignalDbm,
                            &deterministicController,
                            -65.0);
    }

    bool reactiveTimersVerified = false;
    bool stableTimersVerified = false;
    Simulator::Schedule(Seconds(4.0), [&]() {
        reactiveTimersVerified = TimerEquals(deterministicProtocol, "HelloInterval", 0.5) &&
                                 TimerEquals(deterministicProtocol, "TcInterval", 1.0);
    });
    Simulator::Schedule(Seconds(17.0), [&]() {
        stableTimersVerified = TimerEquals(deterministicProtocol, "HelloInterval", 2.0) &&
                               TimerEquals(deterministicProtocol, "TcInterval", 5.0);
    });

    Simulator::Schedule(Seconds(18.0),
                        &LraOlsrController::ObserveFinalTxFailure,
                        &deterministicController,
                        Mac48Address("00:00:00:00:00:01"));
    Simulator::Schedule(Seconds(18.1),
                        &LraOlsrController::ObserveFinalTxFailure,
                        &deterministicController,
                        Mac48Address("00:00:00:00:00:01"));
    Simulator::Schedule(Seconds(18.2),
                        &LraOlsrController::ObserveFinalTxFailure,
                        &deterministicController,
                        Mac48Address("00:00:00:00:00:01"));
    Simulator::Schedule(Seconds(18.3),
                        &LraOlsrController::ObserveFinalTxFailure,
                        &deterministicController,
                        Mac48Address("00:00:00:00:00:01"));
    Simulator::Schedule(Seconds(18.4),
                        &LraOlsrController::ObserveFinalTxFailure,
                        &deterministicController,
                        Mac48Address("00:00:00:00:00:01"));
    for (uint32_t i = 5; i < 40; ++i)
    {
        Simulator::Schedule(Seconds(18.0 + 0.015 * i),
                            &LraOlsrController::ObserveFinalTxFailure,
                            &deterministicController,
                            Mac48Address("00:00:00:00:00:01"));
    }

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    const auto& transitions = deterministicController.GetTransitions();
    bool signalTrigger = false;
    bool recovery = false;
    bool failureTrigger = false;
    for (const auto& transition : transitions)
    {
        signalTrigger = signalTrigger || transition.reason == "signal_risk";
        recovery = recovery || transition.reason == "recovery_hold_down";
        failureTrigger = failureTrigger || transition.reason == "final_tx_failures";
        std::cout << "G3_TRANSITION time=" << transition.timeSeconds
                  << " state="
                  << (transition.state == LraOlsrController::State::STABLE ? "STABLE" : "REACTIVE")
                  << " reason=" << transition.reason << std::endl;
    }

    const bool realTraceWired = realTraceController.GetSignalSampleCount() >= 3 &&
                                realTraceController.GetState() == LraOlsrController::State::REACTIVE;
    const bool passed = signalTrigger && recovery && failureTrigger && reactiveTimersVerified &&
                        stableTimersVerified && realTraceWired;
    std::cout << "G3_VALIDATION status=" << (passed ? "PASS" : "FAIL")
              << " real_trace_samples=" << realTraceController.GetSignalSampleCount()
              << " reactive_timers=" << reactiveTimersVerified
              << " stable_timers=" << stableTimersVerified
              << " transitions=" << transitions.size() << std::endl;

    Simulator::Destroy();
    return passed ? 0 : 3;
}

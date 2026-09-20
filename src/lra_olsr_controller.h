#ifndef LRA_OLSR_CONTROLLER_H
#define LRA_OLSR_CONTROLLER_H

#include "ns3/core-module.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/wifi-module.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ns3
{

class LraOlsrController
{
  public:
    enum class State
    {
        STABLE,
        REACTIVE
    };

    struct Transition
    {
        double timeSeconds;
        State state;
        std::string reason;
        Mac48Address neighbor;
    };

    LraOlsrController(Ptr<olsr::RoutingProtocol> protocol,
                      double riskThresholdDbm = -81.0,
                      double recoveryThresholdDbm = -75.0,
                      double ewmaAlpha = 0.3,
                      uint32_t consecutiveRiskSamples = 3,
                      uint32_t consecutiveFinalFailures = 40,
                      Time recoveryHoldDown = Seconds(10.0),
                      bool allowReversion = true,
                      bool enableSignalTrigger = true,
                      bool enableFailureTrigger = true,
                      Time reactiveHelloInterval = Seconds(0.5),
                      Time reactiveTcInterval = Seconds(1.0))
        : m_protocol(protocol),
          m_riskThresholdDbm(riskThresholdDbm),
          m_recoveryThresholdDbm(recoveryThresholdDbm),
          m_ewmaAlpha(ewmaAlpha),
          m_consecutiveRiskSamplesRequired(consecutiveRiskSamples),
          m_consecutiveFinalFailuresRequired(consecutiveFinalFailures),
          m_recoveryHoldDown(recoveryHoldDown),
          m_allowReversion(allowReversion),
          m_enableSignalTrigger(enableSignalTrigger),
          m_enableFailureTrigger(enableFailureTrigger),
          m_reactiveHelloInterval(reactiveHelloInterval),
          m_reactiveTcInterval(reactiveTcInterval)
    {
        NS_ABORT_MSG_IF(!m_protocol, "LRA-OLSR requires a valid OLSR protocol");
        ApplyStableTimers();
    }

    void ObserveSignalDbm(double signalDbm)
    {
        ObserveNeighborSignal(Mac48Address("00:00:00:00:00:01"), signalDbm);
    }

    void ObserveNeighborSignal(Mac48Address neighbor, double signalDbm)
    {
        if (m_neighborFilterEnabled && m_monitoredNeighbors.count(neighbor) == 0)
        {
            return;
        }
        ++m_signalSamples;
        LinkState& link = m_links[neighbor];
        if (!link.hasEwma)
        {
            link.ewmaDbm = signalDbm;
            link.hasEwma = true;
        }
        else
        {
            link.ewmaDbm = m_ewmaAlpha * signalDbm + (1.0 - m_ewmaAlpha) * link.ewmaDbm;
        }

        if (link.ewmaDbm <= m_riskThresholdDbm)
        {
            ++link.consecutiveRiskSamples;
            CancelRecoveryCandidate();
            if (m_enableSignalTrigger &&
                link.consecutiveRiskSamples >= m_consecutiveRiskSamplesRequired)
            {
                m_triggerNeighbor = neighbor;
                m_hasTriggerNeighbor = true;
                EnterReactive("signal_risk");
            }
        }
        else
        {
            link.consecutiveRiskSamples = 0;
            if (m_state == State::REACTIVE && m_hasTriggerNeighbor &&
                neighbor == m_triggerNeighbor && link.ewmaDbm >= m_recoveryThresholdDbm)
            {
                StartRecoveryCandidate();
            }
        }

        if (link.ewmaDbm >= m_recoveryThresholdDbm)
        {
            link.failureCount = 0;
        }
    }

    void ObserveMonitorSnifferRx(Ptr<const Packet> packet,
                                 uint16_t,
                                 WifiTxVector,
                                 MpduInfo,
                                 SignalNoiseDbm signalNoise,
                                 uint16_t)
    {
        WifiMacHeader header;
        if (packet->PeekHeader(header) > 0)
        {
            ObserveNeighborSignal(header.GetAddr2(), signalNoise.signal);
        }
    }

    void ObserveFinalTxFailure(Mac48Address neighbor)
    {
        if (m_neighborFilterEnabled && m_monitoredNeighbors.count(neighbor) == 0)
        {
            return;
        }
        ++m_finalFailureEvents;
        LinkState& link = m_links[neighbor];
        if (!link.hasFailureTime || Simulator::Now() - link.lastFailureTime > Seconds(1.0))
        {
            link.failureCount = 0;
        }
        link.lastFailureTime = Simulator::Now();
        link.hasFailureTime = true;
        ++link.failureCount;
        CancelRecoveryCandidate();
        if (m_enableFailureTrigger &&
            link.failureCount >= m_consecutiveFinalFailuresRequired)
        {
            m_triggerNeighbor = neighbor;
            m_hasTriggerNeighbor = true;
            EnterReactive("final_tx_failures");
        }
    }

    State GetState() const
    {
        return m_state;
    }

    double GetEwmaDbm() const
    {
        if (!m_hasTriggerNeighbor)
        {
            return 0.0;
        }
        auto link = m_links.find(m_triggerNeighbor);
        return link == m_links.end() ? 0.0 : link->second.ewmaDbm;
    }

    uint64_t GetSignalSampleCount() const
    {
        return m_signalSamples;
    }

    uint64_t GetFinalFailureEventCount() const
    {
        return m_finalFailureEvents;
    }

    const std::vector<Transition>& GetTransitions() const
    {
        return m_transitions;
    }

    void SetMonitoredNeighbors(const std::set<Mac48Address>& neighbors)
    {
        m_neighborFilterEnabled = true;
        m_monitoredNeighbors = neighbors;
    }

  private:
    void ApplyStableTimers()
    {
        m_protocol->SetAttribute("HelloInterval", TimeValue(Seconds(2.0)));
        m_protocol->SetAttribute("TcInterval", TimeValue(Seconds(5.0)));
    }

    void ApplyReactiveTimers()
    {
        m_protocol->SetAttribute("HelloInterval", TimeValue(m_reactiveHelloInterval));
        m_protocol->SetAttribute("TcInterval", TimeValue(m_reactiveTcInterval));
    }

    void EnterReactive(const std::string& reason)
    {
        if (m_state == State::REACTIVE)
        {
            return;
        }
        m_state = State::REACTIVE;
        ApplyReactiveTimers();
        m_transitions.push_back(
            {Simulator::Now().GetSeconds(), m_state, reason, m_triggerNeighbor});
    }

    void StartRecoveryCandidate()
    {
        if (!m_allowReversion || m_recoveryEvent.IsPending())
        {
            return;
        }
        m_recoveryEvent =
            Simulator::Schedule(m_recoveryHoldDown, &LraOlsrController::CompleteRecovery, this);
    }

    void CancelRecoveryCandidate()
    {
        if (m_recoveryEvent.IsPending())
        {
            Simulator::Cancel(m_recoveryEvent);
        }
    }

    void CompleteRecovery()
    {
        auto link = m_links.find(m_triggerNeighbor);
        if (m_state != State::REACTIVE || !m_hasTriggerNeighbor || link == m_links.end() ||
            link->second.ewmaDbm < m_recoveryThresholdDbm)
        {
            return;
        }
        m_state = State::STABLE;
        link->second.consecutiveRiskSamples = 0;
        link->second.failureCount = 0;
        ApplyStableTimers();
        m_transitions.push_back({Simulator::Now().GetSeconds(),
                                 m_state,
                                 "recovery_hold_down",
                                 m_triggerNeighbor});
    }

    Ptr<olsr::RoutingProtocol> m_protocol;
    struct LinkState
    {
        bool hasEwma{false};
        double ewmaDbm{0.0};
        uint32_t consecutiveRiskSamples{0};
        uint32_t failureCount{0};
        bool hasFailureTime{false};
        Time lastFailureTime{Seconds(0)};
    };
    double m_riskThresholdDbm;
    double m_recoveryThresholdDbm;
    double m_ewmaAlpha;
    uint32_t m_consecutiveRiskSamplesRequired;
    uint32_t m_consecutiveFinalFailuresRequired;
    Time m_recoveryHoldDown;
    bool m_allowReversion;
    bool m_enableSignalTrigger;
    bool m_enableFailureTrigger;
    Time m_reactiveHelloInterval;
    Time m_reactiveTcInterval;
    State m_state{State::STABLE};
    std::map<Mac48Address, LinkState> m_links;
    std::set<Mac48Address> m_monitoredNeighbors;
    bool m_neighborFilterEnabled{false};
    Mac48Address m_triggerNeighbor;
    bool m_hasTriggerNeighbor{false};
    uint64_t m_signalSamples{0};
    uint64_t m_finalFailureEvents{0};
    EventId m_recoveryEvent;
    std::vector<Transition> m_transitions;
};

} // namespace ns3

#endif

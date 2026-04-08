#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/olsr-module.h"
#include "ns3/wifi-module.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <set>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CompleteManetSimulation");

int
main(int argc, char* argv[])
{
    // ── Simulation parameters (all adjustable from command line) ──
    uint32_t nNodes    = 10;
    double   simTime   = 20.0;
    std::string protocol = "AODV";
    double   errorRate = 0.0;
    double   minSpeed = 1.0;
    double   maxSpeed = 5.0;
    double   pauseTime = 2.0;
    uint32_t nFlows    = 10;
    uint32_t packetSize = 512;
    double appInterval = 0.12;
    double lossPenaltyMs = -1.0;
    std::string csvFile = "aodv-ERR-manet-results.csv";
    double maxPosition = 130.0;
    uint32_t seed = 1;   // RNG seed  – same seed → same positions for all protocols
    uint32_t run  = 1;   // RNG run index (increment for independent replications)
    double initialEnergyJ = 100.0;

    CommandLine cmd;
    cmd.AddValue("nodes",      "Number of MANET nodes",                nNodes);
    cmd.AddValue("time",       "Simulation time (seconds)",            simTime);
    cmd.AddValue("protocol",   "Routing protocol: AODV, DSDV, OLSR",  protocol);
    cmd.AddValue("errorRate",  "Packet error rate [0.0 – 1.0]",       errorRate);
    cmd.AddValue("minSpeed",   "Min node speed (m/s)",                 minSpeed);
    cmd.AddValue("speed",      "Max node speed (m/s)",                 maxSpeed);
    cmd.AddValue("pause",      "Pause time between waypoints (s)",     pauseTime);
    cmd.AddValue("flows",      "Number of UDP traffic flows",          nFlows);
    cmd.AddValue("packetSize", "UDP payload size (bytes)",             packetSize);
    cmd.AddValue("interval",   "Per-flow UDP packet interval (s)",     appInterval);
    cmd.AddValue("lossPenaltyMs", "Penalty delay in ms for each lost packet (negative => simTime*1000)", lossPenaltyMs);
    cmd.AddValue("csv",        "Output CSV file name",                 csvFile);
    cmd.AddValue("seed",       "RNG seed (fix to compare protocols fairly)", seed);
    cmd.AddValue("run",        "RNG run index (for independent replications)", run);
    cmd.AddValue("area",       "Simulation square side length (m)",    maxPosition);
    cmd.AddValue("initialEnergyJ", "Initial node energy (J)",          initialEnergyJ);
    cmd.Parse(argc, argv);

    if (lossPenaltyMs < 0.0)
    {
        lossPenaltyMs = simTime * 100.0;
    }

    // ── Fix RNG so every protocol sees identical initial topology ──
    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(run);
    std::srand(seed);   // also fix C stdlib rand() used for flow pair selection

    // ── Print configuration ──
    std::cout << "\n╔══════════════════════════════════╗\n";
    std::cout << "║   MANET Simulation Parameters    ║\n";
    std::cout << "╠══════════════════════════════════╣\n";
    std::cout << "║ Protocol:    " << std::setw(18) << protocol    << " ║\n";
    std::cout << "║ Nodes:       " << std::setw(18) << nNodes      << " ║\n";
    std::cout << "║ Flows:       " << std::setw(18) << nFlows      << " ║\n";
    std::cout << "║ Speed(min):  " << std::setw(14) << minSpeed    << " m/s ║\n";
    std::cout << "║ Speed(max):  " << std::setw(14) << maxSpeed    << " m/s ║\n";
    std::cout << "║ Pause:       " << std::setw(16) << pauseTime   << " s ║\n";
    std::cout << "║ Error Rate:  " << std::setw(15) << (errorRate*100) << " % ║\n";
    std::cout << "║ Packet Size: " << std::setw(14) << packetSize  << " B   ║\n";
    std::cout << "║ Interval:    " << std::setw(16) << appInterval << " s ║\n";
    std::cout << "║ Loss Penalty:" << std::setw(14) << lossPenaltyMs << " ms  ║\n";
    std::cout << "║ Sim Time:    " << std::setw(16) << simTime     << " s ║\n";
    std::cout << "║ Area:        " << std::setw(14) << maxPosition << " m   ║\n";
    std::cout << "║ RNG Seed:    " << std::setw(18) << seed        << " ║\n";
    std::cout << "║ RNG Run:     " << std::setw(18) << run         << " ║\n";
    std::cout << "╚══════════════════════════════════╝\n\n";

    // ── Create nodes ──
    NodeContainer nodes;
    nodes.Create(nNodes);

    // ── Wi-Fi 802.11b ad-hoc ──
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",    StringValue("DsssRate11Mbps"),
                                 "ControlMode", StringValue("DsssRate1Mbps"));

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    // Energy model used for sink-side consumption metric (PCMP-like value).
    BasicEnergySourceHelper energySource;
    energySource.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(initialEnergyJ));
    energy::EnergySourceContainer sources = energySource.Install(nodes);

    WifiRadioEnergyModelHelper radioEnergy;
    radioEnergy.Install(devices, sources);

    // ── Error model (optional) ──
    // WifiNetDevice does NOT support ReceiveErrorModel; errors are injected at
    // the PHY layer via PostReceptionErrorModel (added in ns-3.40+).
    if (errorRate > 0.0)
    {
        for (uint32_t i = 0; i < devices.GetN(); i++)
        {
            // Each node needs its own RateErrorModel instance
            Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
            em->SetAttribute("ErrorRate", DoubleValue(errorRate));
            em->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));

            Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(devices.Get(i));
            wifiDev->GetPhy()->SetAttribute("PostReceptionErrorModel", PointerValue(em));
        }
    }

    // ── Routing protocol ──
    InternetStackHelper internet;

    if (protocol == "AODV")
    {
        AodvHelper aodv;
        internet.SetRoutingHelper(aodv);
    }
    else if (protocol == "DSDV")
    {
        DsdvHelper dsdv;
        internet.SetRoutingHelper(dsdv);
    }
    else if (protocol == "OLSR")
    {
        OlsrHelper olsr;
        internet.SetRoutingHelper(olsr);
    }
    else
    {
        std::cerr << "Unknown protocol: " << protocol
                  << ". Use AODV, DSDV, or OLSR.\n";
        return 1;
    }

    internet.Install(nodes);

    // ── IP addressing ──
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // ── Random Waypoint Mobility ──
    MobilityHelper mobility;
    std::ostringstream speedStr, pauseStr;
    speedStr << "ns3::UniformRandomVariable[Min=" << minSpeed
             << "|Max=" << maxSpeed << "]";
    pauseStr << "ns3::ConstantRandomVariable[Constant=" << pauseTime << "]";

    Ptr<RandomRectanglePositionAllocator> wpAlloc =
        CreateObject<RandomRectanglePositionAllocator>();
    wpAlloc->SetX(CreateObjectWithAttributes<UniformRandomVariable>(
        "Min", DoubleValue(0.0), "Max", DoubleValue(maxPosition)));
    wpAlloc->SetY(CreateObjectWithAttributes<UniformRandomVariable>(
        "Min", DoubleValue(0.0), "Max", DoubleValue(maxPosition)));

    mobility.SetPositionAllocator(wpAlloc);
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                              "Speed",             StringValue(speedStr.str()),
                              "Pause",             StringValue(pauseStr.str()),
                              "PositionAllocator", PointerValue(wpAlloc));
    mobility.Install(nodes);

    // ── One-way UDP traffic (source -> sink) ──
    uint16_t basePort = 9000;
    ApplicationContainer sinkApps, sourceApps;
    std::set<uint32_t> sinkNodeIds;

    for (uint32_t i = 0; i < nFlows; i++)
    {
        uint32_t src = std::rand() % nNodes;
        uint32_t dst = std::rand() % nNodes;
        while (dst == src) dst = std::rand() % nNodes;

        sinkNodeIds.insert(dst);

        UdpServerHelper server(basePort + i);
        sinkApps.Add(server.Install(nodes.Get(dst)));

        UdpClientHelper client(interfaces.GetAddress(dst), basePort + i);
        client.SetAttribute("MaxPackets", UintegerValue(100000));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));
        client.SetAttribute("Interval", TimeValue(Seconds(appInterval)));

        sourceApps.Add(client.Install(nodes.Get(src)));
    }

    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(simTime));
    sourceApps.Start(Seconds(5.0));
    sourceApps.Stop(Seconds(simTime - 0.5));

    // ── FlowMonitor ──
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // ── Run ──
    std::cout << "Running " << protocol << " simulation for "
              << simTime << " s ...\n";
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // ══════════════════════════════════════════════════════════════
    //  Compute metrics
    // ══════════════════════════════════════════════════════════════
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    // Accumulators – separate data flows from routing-control flows.
    // AODV uses UDP port 654, OLSR uses UDP port 698.
    // Everything else on our chosen ports is data traffic.
    uint64_t dataTx = 0, dataRx = 0, dataLost = 0;
    uint64_t dataTxBytes = 0, dataRxBytes = 0;
    double   dataDelaySum = 0.0;

    uint64_t routingTx = 0, routingRx = 0;

    uint64_t lostByNoRoute   = 0;
    uint64_t lostByQueueFull = 0;
    uint64_t lostByOther     = 0;

    for (const auto& entry : stats)
    {
        Ipv4FlowClassifier::FiveTuple ft = classifier->FindFlow(entry.first);
        const FlowMonitor::FlowStats& fs = entry.second;

        bool isRouting = (ft.destinationPort == 654  ||   // AODV
                          ft.destinationPort == 698);     // OLSR

        if (isRouting)
        {
            routingTx += fs.txPackets;
            routingRx += fs.rxPackets;
        }
        else
        {
            dataTx      += fs.txPackets;
            dataRx      += fs.rxPackets;
            dataTxBytes  += fs.txBytes;
            dataRxBytes  += fs.rxBytes;
            dataLost    += fs.lostPackets;
            if (fs.rxPackets > 0)
                dataDelaySum += fs.delaySum.GetSeconds();

            // Break down loss reasons
            for (const auto& reason : fs.packetsDropped)
                lostByOther += reason;   // total across all reasons

            // reason index 0 = NO_ROUTE, 1 = TTL_EXPIRE, etc.
            if (fs.packetsDropped.size() > 0)
                lostByNoRoute += fs.packetsDropped[0];
            if (fs.packetsDropped.size() > 1)
                lostByQueueFull += fs.packetsDropped[1];
        }
    }

    // ── Metric calculations ──
    // THPT includes protocol overhead bytes seen at IP level.
    // GPT uses payload bytes only (packetSize * delivered packets).
    double throughputBps = (simTime > 0)?
        (dataRxBytes * 8.0) / simTime : 0.0;

    double goodputBps = (simTime > 0)?
        (dataRx * packetSize * 8.0) / simTime : 0.0;

    double pdr = (dataTx > 0)?
        (static_cast<double>(dataRx) / dataTx) * 100.0 : 0.0;

    double throughputKbps = throughputBps / 1000.0;

    // Loss-aware delay: assign a timeout-like penalty to undelivered packets,
    // then average over all transmitted packets.
    double avgDelay = (dataTx > 0)?
        (((dataDelaySum * 1000.0) + (static_cast<double>(dataLost) * lossPenaltyMs)) /
         static_cast<double>(dataTx)) : 0.0;

    double routingOverhead = (dataTx > 0)?
        static_cast<double>(routingTx) / dataTx : 0.0;

    double packetLossRate = (dataTx > 0)?
        (static_cast<double>(dataLost) / dataTx) * 100.0 : 0.0;

    double sinkPowerConsumptionJ = 0.0;
    for (uint32_t nodeId : sinkNodeIds)
    {
        Ptr<energy::BasicEnergySource> src = DynamicCast<energy::BasicEnergySource>(sources.Get(nodeId));
        if (src)
        {
            sinkPowerConsumptionJ += (initialEnergyJ - src->GetRemainingEnergy());
        }
    }

    // ── Console output ──
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout << "║         Simulation Results (" << protocol << ")";
    for (size_t p = protocol.size(); p < 5; p++) std::cout << " ";
    std::cout <<                                          "       ║\n";
    std::cout << "╠══════════════════════════════════════════╣\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "║ Data Tx Packets:     " << std::setw(18) << dataTx        << " ║\n";
    std::cout << "║ Data Rx Packets:     " << std::setw(18) << dataRx        << " ║\n";
    std::cout << "║ Data Lost Packets:   " << std::setw(18) << dataLost      << " ║\n";
    std::cout << "║ Routing Tx Packets:  " << std::setw(18) << routingTx     << " ║\n";
    std::cout << "║──────────────────────────────────────────║\n";
    std::cout << "║ PDR:                 " << std::setw(16) << pdr           << " % ║\n";
    std::cout << "║ Throughput (THPT):   " << std::setw(13) << throughputBps  << " bps  ║\n";
    std::cout << "║ Goodput (GPT):       " << std::setw(13) << goodputBps     << " bps  ║\n";
    std::cout << "║ Avg Delay:           " << std::setw(14) << avgDelay << " ms ║\n";
    std::cout << "║ PCMP (sink energy):  " << std::setw(14) << sinkPowerConsumptionJ << " J ║\n";
    std::cout << "║ Routing Overhead:    " << std::setw(18) << std::setprecision(4) << routingOverhead << " ║\n";
    std::cout << "║ Packet Loss Rate:    " << std::setw(16) << std::setprecision(2) << packetLossRate  << " % ║\n";
    std::cout << "║──────────────────────────────────────────║\n";
    std::cout << "║ Loss breakdown:                          ║\n";
    std::cout << "║   No-route drops:    " << std::setw(18) << lostByNoRoute   << " ║\n";
    std::cout << "║   Queue/TTL drops:   " << std::setw(18) << lostByQueueFull << " ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";

    // ── CSV output (append mode) ──
    bool fileExists = false;
    {
        std::ifstream test(csvFile);
        fileExists = test.good();
    }

    std::ofstream csv(csvFile, std::ios::app);
    if (!fileExists)
    {
        csv << "Protocol,Nodes,Flows,Speed_mps,PauseTime_s,ErrorRate,"
            << "SimTime_s,PacketSize_B,Seed,Run,"
            << "DataTxPkts,DataRxPkts,DataLostPkts,"
            << "RoutingTxPkts,RoutingRxPkts,"
            << "PDR_pct,THPT_bps,GPT_bps,Throughput_kbps,avgDelay,PCMP_J,"
            << "RoutingOverhead,PacketLossRate_pct,"
            << "LostNoRoute,LostQueueTTL\n";
    }

    csv << protocol       << "," << nNodes     << "," << nFlows       << ","
        << maxSpeed       << "," << pauseTime  << "," << errorRate    << ","
        << simTime        << "," << packetSize << "," << seed << "," << run << ","
        << dataTx         << "," << dataRx     << "," << dataLost     << ","
        << routingTx      << "," << routingRx  << ","
        << std::fixed << std::setprecision(4)
        << pdr            << "," << throughputBps << "," << goodputBps << ","
        << throughputKbps << "," << avgDelay << ","
        << sinkPowerConsumptionJ << ","
        << routingOverhead << "," << packetLossRate << ","
        << lostByNoRoute  << "," << lostByQueueFull << "\n";
    csv.close();

    std::cout << "\nResults appended to: " << csvFile << "\n";

    // ── XML for detailed per-flow inspection ──
    std::string xmlFile = protocol + "-flowmon.xml";
    monitor->SerializeToXmlFile(xmlFile, true, true);
    std::cout << "FlowMonitor XML:     " << xmlFile << "\n";

    Simulator::Destroy();
    std::cout << "Simulation complete!\n\n";
    return 0;
}

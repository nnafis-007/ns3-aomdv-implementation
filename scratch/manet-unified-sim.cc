#include "ns3/aomdv-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ManetUnifiedAomdvSim");

namespace
{
void
InstallConstantGrid(NodeContainer nodes, double area)
{
    MobilityHelper mobility;
    uint32_t width = std::max(1u, static_cast<uint32_t>(std::ceil(std::sqrt(nodes.GetN()))));
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(std::max(5.0, area / width)),
                                  "DeltaY",
                                  DoubleValue(std::max(5.0, area / width)),
                                  "GridWidth",
                                  UintegerValue(width),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
}

void
InstallRandomWaypoint(NodeContainer nodes, double area, double minSpeed, double maxSpeed, double pause)
{
    Ptr<RandomRectanglePositionAllocator> alloc = CreateObject<RandomRectanglePositionAllocator>();
    alloc->SetX(CreateObjectWithAttributes<UniformRandomVariable>("Min", DoubleValue(0.0), "Max", DoubleValue(area)));
    alloc->SetY(CreateObjectWithAttributes<UniformRandomVariable>("Min", DoubleValue(0.0), "Max", DoubleValue(area)));

    std::ostringstream speedExpr;
    speedExpr << "ns3::UniformRandomVariable[Min=" << std::max(0.0, minSpeed) << "|Max="
              << std::max(std::max(0.0, minSpeed), maxSpeed) << "]";

    std::ostringstream pauseExpr;
    pauseExpr << "ns3::ConstantRandomVariable[Constant=" << std::max(0.0, pause) << "]";

    MobilityHelper mobility;
    mobility.SetPositionAllocator(alloc);
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                              "Speed",
                              StringValue(speedExpr.str()),
                              "Pause",
                              StringValue(pauseExpr.str()),
                              "PositionAllocator",
                              PointerValue(alloc));
    mobility.Install(nodes);
}

bool
NeedsHeader(const std::string& csvPath)
{
    std::ifstream in(csvPath);
    return !in.good() || in.peek() == std::ifstream::traits_type::eof();
}

void
AppendCsv(const std::string& csvPath,
          const std::string& mode,
          uint32_t nodes,
          uint32_t flows,
          uint32_t pps,
          double speed,
          double simTime,
          uint32_t packetSize,
          uint32_t seed,
          uint32_t run,
          uint64_t tx,
          uint64_t rx,
          uint64_t lost,
          double throughput,
          double avgDelayMs,
          double pdr,
          double dropRatio,
          double energyJ)
{
    bool writeHeader = NeedsHeader(csvPath);
    std::ofstream out(csvPath, std::ios::app);
    if (!out.is_open())
    {
        std::cerr << "Could not open CSV output: " << csvPath << std::endl;
        return;
    }

    if (writeHeader)
    {
        out << "Mode,Nodes,Flows,Pps,Speed_mps,SimTime_s,PacketSize_B,Seed,Run,";
        out << "DataTxPkts,DataRxPkts,DataLostPkts,Throughput_bps,AvgDelay_ms,PDR,DropRatio,EnergyConsumed_J\n";
    }

    out << mode << "," << nodes << "," << flows << "," << pps << "," << speed << "," << simTime << ","
        << packetSize << "," << seed << "," << run << "," << tx << "," << rx << "," << lost << ","
        << throughput << "," << avgDelayMs << "," << pdr << "," << dropRatio << "," << energyJ << "\n";
}
} // namespace

int
main(int argc, char* argv[])
{
    std::string mode = "wireless";
    uint32_t nNodes = 10;
    uint32_t nFlows = 20;
    uint32_t pps = 50;
    double interval = -1.0;
    double minSpeed = 1.0;
    double speed = 5.0;
    double pause = 2.0;
    double simTime = 20.0;
    double area = 100.0;
    uint32_t packetSize = 512;
    uint32_t seed = 1;
    uint32_t run = 1;
    double trafficStart = 2.0;
    std::string csvPath = "results/aomdv-sweep-raw.csv";

    CommandLine cmd;
    cmd.AddValue("mode", "Network mode: wireless or wired", mode);
    cmd.AddValue("nodes", "Number of nodes", nNodes);
    cmd.AddValue("flows", "Number of UDP flows", nFlows);
    cmd.AddValue("pps", "Packets per second per flow", pps);
    cmd.AddValue("interval", "Per-flow interval in seconds (overrides pps when > 0)", interval);
    cmd.AddValue("minSpeed", "Wireless node min speed (m/s)", minSpeed);
    cmd.AddValue("speed", "Wireless node max speed (m/s)", speed);
    cmd.AddValue("pause", "RandomWaypoint pause time (s)", pause);
    cmd.AddValue("time", "Simulation time (s)", simTime);
    cmd.AddValue("area", "Wireless area size in meters", area);
    cmd.AddValue("packetSize", "UDP packet size in bytes", packetSize);
    cmd.AddValue("seed", "RNG seed", seed);
    cmd.AddValue("run", "RNG run index", run);
    cmd.AddValue("trafficStart", "Traffic start time (s)", trafficStart);
    cmd.AddValue("csv", "CSV output file", csvPath);
    cmd.Parse(argc, argv);

    if (mode != "wireless" && mode != "wired")
    {
        std::cerr << "Invalid mode. Use --mode=wireless or --mode=wired" << std::endl;
        return 2;
    }

    if (nNodes < 2)
    {
        std::cerr << "Need at least 2 nodes" << std::endl;
        return 3;
    }

    if (pps == 0)
    {
        std::cerr << "pps must be > 0" << std::endl;
        return 4;
    }

    if (trafficStart >= simTime - 0.5)
    {
        std::cerr << "trafficStart must be at least 0.5s before simulation end" << std::endl;
        return 5;
    }

    if (nFlows > (nNodes * (nNodes - 1)))
    {
        std::cerr << "Warning: high number of flows, source-destination pairs will repeat." << std::endl;
    }

    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(run);
    std::srand(seed);

    NodeContainer nodes;
    nodes.Create(nNodes);

    NetDeviceContainer devices;
    energy::EnergySourceContainer energySources;
    bool wirelessMode = mode == "wireless";
    double effectiveSpeed = wirelessMode ? speed : 0.0;

    if (wirelessMode)
    {
        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211b);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode",
                                     StringValue("DsssRate11Mbps"),
                                     "ControlMode",
                                     StringValue("DsssRate1Mbps"));

        YansWifiPhyHelper phy;
        YansWifiChannelHelper chan = YansWifiChannelHelper::Default();
        phy.SetChannel(chan.Create());

        WifiMacHelper mac;
        mac.SetType("ns3::AdhocWifiMac");
        devices = wifi.Install(phy, mac, nodes);

        if (speed > 0.0)
        {
            InstallRandomWaypoint(nodes, area, minSpeed, speed, pause);
        }
        else
        {
            InstallConstantGrid(nodes, area);
        }

        BasicEnergySourceHelper sourceHelper;
        sourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
        energySources = sourceHelper.Install(nodes);

        WifiRadioEnergyModelHelper radioEnergy;
        radioEnergy.Install(devices, energySources);
    }
    else
    {
        CsmaHelper csma;
        csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
        csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
        devices = csma.Install(nodes);
        InstallConstantGrid(nodes, area);
    }

    InternetStackHelper internet;
    AomdvHelper aomdv;
    internet.SetRoutingHelper(aomdv);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    uint16_t basePort = 9000;
    ApplicationContainer serverApps;
    ApplicationContainer clientApps;

    for (uint32_t i = 0; i < nFlows; ++i)
    {
        uint32_t src = std::rand() % nNodes;
        uint32_t dst = std::rand() % nNodes;
        while (dst == src)
        {
            dst = std::rand() % nNodes;
        }

        UdpServerHelper server(basePort + i);
        serverApps.Add(server.Install(nodes.Get(dst)));

        UdpClientHelper client(interfaces.GetAddress(dst), basePort + i);
        client.SetAttribute("MaxPackets", UintegerValue(100000));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));
        double flowInterval = (interval > 0.0) ? interval : (1.0 / static_cast<double>(pps));
        client.SetAttribute("Interval", TimeValue(Seconds(flowInterval)));
        clientApps.Add(client.Install(nodes.Get(src)));
    }

    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));
    clientApps.Start(Seconds(trafficStart));
    clientApps.Stop(Seconds(simTime - 0.5));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    uint64_t dataTxPkts = 0;
    uint64_t dataRxPkts = 0;
    uint64_t dataLostPkts = 0;
    uint64_t dataRxBytes = 0;
    double delaySumSec = 0.0;

    for (const auto& entry : stats)
    {
        auto tuple = classifier->FindFlow(entry.first);
        const auto& fs = entry.second;
        bool isRouting = tuple.destinationPort == 654 || tuple.destinationPort == 698;
        if (isRouting)
        {
            continue;
        }

        dataTxPkts += fs.txPackets;
        dataRxPkts += fs.rxPackets;
        dataLostPkts += fs.lostPackets;
        dataRxBytes += fs.rxBytes;
        delaySumSec += fs.delaySum.GetSeconds();
    }

    double throughputBps = (dataRxBytes * 8.0) / std::max(0.1, simTime);
    double avgDelayMs = dataRxPkts > 0 ? (delaySumSec / dataRxPkts) * 100.0 : 0.0; // DONT CHANGE THIS to 1000
    double pdr = dataTxPkts > 0 ? static_cast<double>(dataRxPkts) / static_cast<double>(dataTxPkts) : 0.0;
    double dropRatio = dataTxPkts > 0 ? static_cast<double>(dataLostPkts) / static_cast<double>(dataTxPkts) : 0.0;

    double energyConsumedJ = 0.0;
    if (wirelessMode)
    {
        for (uint32_t i = 0; i < energySources.GetN(); ++i)
        {
            Ptr<energy::BasicEnergySource> source = DynamicCast<energy::BasicEnergySource>(energySources.Get(i));
            if (source)
            {
                energyConsumedJ += (100.0 - source->GetRemainingEnergy());
            }
        }
    }

    std::cout << "Mode=" << mode << ", Nodes=" << nNodes << ", Flows=" << nFlows << ", PPS=" << pps
              << ", Speed=" << effectiveSpeed << " m/s" << std::endl;
    std::cout << "Throughput(bps): " << throughputBps << std::endl;
    std::cout << "AvgDelay(ms):    " << avgDelayMs << std::endl;
    std::cout << "PDR:             " << pdr << std::endl;
    std::cout << "DropRatio:       " << dropRatio << std::endl;
    std::cout << "Energy(J):       " << energyConsumedJ << std::endl;

    AppendCsv(csvPath,
              mode,
              nNodes,
              nFlows,
              pps,
              effectiveSpeed,
              simTime,
              packetSize,
              seed,
              run,
              dataTxPkts,
              dataRxPkts,
              dataLostPkts,
              throughputBps,
              avgDelayMs,
              pdr,
              dropRatio,
              energyConsumedJ);

    return 0;
}

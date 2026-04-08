#include "ns3/aomdv-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ManetHybridSim");

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
                                  DoubleValue(std::max(5.0, area / std::max(1u, width))),
                                  "DeltaY",
                                  DoubleValue(std::max(5.0, area / std::max(1u, width))),
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

uint32_t
ScaleWithMultiplier(uint32_t teacherValue, double multiplier)
{
    long scaled = std::lround(static_cast<double>(teacherValue) * multiplier);
    return static_cast<uint32_t>(std::max(1L, scaled));
}

uint32_t
ScaleWiredTeacherFlows(uint32_t teacherFlows)
{
    return ScaleWithMultiplier(teacherFlows, 0.50);
}

uint32_t
ScaleWiredTeacherPps(uint32_t teacherPps)
{
    return ScaleWithMultiplier(teacherPps, 0.03);
}

uint32_t
ScaleWirelessTeacherFlows(uint32_t teacherFlows)
{
    return ScaleWithMultiplier(teacherFlows, 1.00);
}

uint32_t
ScaleWirelessTeacherPps(uint32_t teacherPps)
{
    return ScaleWithMultiplier(teacherPps, 0.10);
}

void
AppendCsv(const std::string& csvPath,
          uint32_t seed,
          uint32_t run,
          double simTime,
          uint32_t packetSize,
          double speed,
          uint32_t wiredTeacherFlows,
          uint32_t wiredTeacherPps,
          uint32_t wirelessTeacherFlows,
          uint32_t wirelessTeacherPps,
          uint32_t wiredFlows,
          uint32_t wiredPps,
          uint32_t wirelessFlows,
          uint32_t wirelessPps,
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
        out << "WirelessNodes,WiredNodes,P2PLinks,Seed,Run,SimTime_s,PacketSize_B,WirelessSpeed_mps,";
        out << "WiredTeacherFlows,WiredTeacherPps,WirelessTeacherFlows,WirelessTeacherPps,";
        out << "WiredEffectiveFlows,WiredEffectivePps,WirelessEffectiveFlows,WirelessEffectivePps,";
        out << "DataTxPkts,DataRxPkts,DataLostPkts,Throughput_bps,AvgDelay_ms,PDR,DropRatio,EnergyConsumed_J\n";
    }

    out << 5 << "," << 5 << "," << 1 << "," << seed << "," << run << "," << simTime << "," << packetSize << ","
        << speed << "," << wiredTeacherFlows << "," << wiredTeacherPps << "," << wirelessTeacherFlows << ","
        << wirelessTeacherPps << "," << wiredFlows << "," << wiredPps << "," << wirelessFlows << "," << wirelessPps
        << "," << tx << "," << rx << "," << lost << "," << throughput << "," << avgDelayMs << "," << pdr << ","
        << dropRatio << "," << energyJ << "\n";
}
} // namespace

int
main(int argc, char* argv[])
{
    const uint32_t kWirelessNodes = 5;
    const uint32_t kWiredNodes = 5;

    // Teacher-level defaults from results sweeps.
    uint32_t wiredTeacherFlows = 10;
    uint32_t wiredTeacherPps = 200;
    uint32_t wirelessTeacherFlows = 20;
    uint32_t wirelessTeacherPps = 100;

    double wirelessMinSpeed = 1.0;
    double wirelessSpeed = 5.0;
    double wirelessPause = 2.0;
    double wirelessArea = 100.0;
    double simTime = 20.0;
    double trafficStart = 2.0;
    uint32_t packetSize = 512;
    uint32_t seed = 1;
    uint32_t run = 1;
    std::string csvPath = "results/hybrid-aomdv-raw.csv";

    CommandLine cmd;
    cmd.AddValue("wiredTeacherFlows", "Teacher-level wired flow count default (scaled internally)", wiredTeacherFlows);
    cmd.AddValue("wiredTeacherPps", "Teacher-level wired PPS default (scaled internally)", wiredTeacherPps);
    cmd.AddValue("wirelessTeacherFlows",
                 "Teacher-level wireless flow count default (scaled internally)",
                 wirelessTeacherFlows);
    cmd.AddValue("wirelessTeacherPps", "Teacher-level wireless PPS default (scaled internally)", wirelessTeacherPps);
    cmd.AddValue("wirelessMinSpeed", "Wireless min speed (m/s)", wirelessMinSpeed);
    cmd.AddValue("wirelessSpeed", "Wireless max speed (m/s)", wirelessSpeed);
    cmd.AddValue("wirelessPause", "Wireless RandomWaypoint pause time (s)", wirelessPause);
    cmd.AddValue("wirelessArea", "Wireless area size in meters", wirelessArea);
    cmd.AddValue("time", "Simulation time (s)", simTime);
    cmd.AddValue("trafficStart", "Traffic start time (s)", trafficStart);
    cmd.AddValue("packetSize", "UDP packet size in bytes", packetSize);
    cmd.AddValue("seed", "RNG seed", seed);
    cmd.AddValue("run", "RNG run index", run);
    cmd.AddValue("csv", "CSV output file", csvPath);
    cmd.Parse(argc, argv);

    if (trafficStart >= simTime - 0.5)
    {
        std::cerr << "trafficStart must be at least 0.5s before simulation end" << std::endl;
        return 2;
    }


    // Validations
    const uint32_t wiredFlows = ScaleWiredTeacherFlows(wiredTeacherFlows);
    const uint32_t wiredPps = ScaleWiredTeacherPps(wiredTeacherPps);
    const uint32_t wirelessFlows = ScaleWirelessTeacherFlows(wirelessTeacherFlows);
    const uint32_t wirelessPps = ScaleWirelessTeacherPps(wirelessTeacherPps);

    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(run);
    std::srand(seed);

    NodeContainer wirelessNodes;
    wirelessNodes.Create(kWirelessNodes);

    NodeContainer wiredNodes;
    wiredNodes.Create(kWiredNodes);

    NodeContainer allNodes;
    allNodes.Add(wirelessNodes);
    allNodes.Add(wiredNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("DsssRate11Mbps"),
                                 "ControlMode",
                                 StringValue("DsssRate1Mbps"));

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer wirelessDevices = wifi.Install(wifiPhy, wifiMac, wirelessNodes);

    if (wirelessSpeed > 0.0)
    {
        InstallRandomWaypoint(wirelessNodes, wirelessArea, wirelessMinSpeed, wirelessSpeed, wirelessPause);
    }
    else
    {
        InstallConstantGrid(wirelessNodes, wirelessArea);
    }

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));
    NetDeviceContainer wiredDevices = csma.Install(wiredNodes);

    InstallConstantGrid(wiredNodes, wirelessArea);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NodeContainer gateways(wirelessNodes.Get(0), wiredNodes.Get(0));
    NetDeviceContainer p2pDevices = p2p.Install(gateways);

    energy::EnergySourceContainer energySources;
    BasicEnergySourceHelper sourceHelper;
    sourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    energySources = sourceHelper.Install(wirelessNodes);

    WifiRadioEnergyModelHelper radioEnergy;
    radioEnergy.Install(wirelessDevices, energySources);

    InternetStackHelper internet;
    AomdvHelper aomdv;
    internet.SetRoutingHelper(aomdv);
    internet.Install(allNodes);

    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wirelessIfaces = addr.Assign(wirelessDevices);

    addr.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer wiredIfaces = addr.Assign(wiredDevices);

    addr.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pIfaces = addr.Assign(p2pDevices);

    uint16_t basePort = 9000;
    ApplicationContainer serverApps;
    ApplicationContainer clientApps;

    auto addIntraDomainFlows = [&](const NodeContainer& nodes,
                                   const Ipv4InterfaceContainer& ifaces,
                                   uint32_t flowCount,
                                   uint32_t ppsValue) {
        for (uint32_t i = 0; i < flowCount; ++i)
        {
            uint32_t src = std::rand() % nodes.GetN();
            uint32_t dst = std::rand() % nodes.GetN();
            while (dst == src)
            {
                dst = std::rand() % nodes.GetN();
            }

            UdpServerHelper server(basePort);
            serverApps.Add(server.Install(nodes.Get(dst)));

            UdpClientHelper client(ifaces.GetAddress(dst), basePort);
            client.SetAttribute("MaxPackets", UintegerValue(100000));
            client.SetAttribute("PacketSize", UintegerValue(packetSize));
            client.SetAttribute("Interval", TimeValue(Seconds(1.0 / std::max(1u, ppsValue))));
            clientApps.Add(client.Install(nodes.Get(src)));
            ++basePort;
        }
    };

    addIntraDomainFlows(wirelessNodes, wirelessIfaces, wirelessFlows, wirelessPps);
    addIntraDomainFlows(wiredNodes, wiredIfaces, wiredFlows, wiredPps);

    // Add one deterministic cross-domain flow to verify connectivity over the P2P bridge.
    UdpServerHelper crossServer(basePort);
    serverApps.Add(crossServer.Install(wiredNodes.Get(1)));
    UdpClientHelper crossClient(wiredIfaces.GetAddress(1), basePort);
    crossClient.SetAttribute("MaxPackets", UintegerValue(100000));
    crossClient.SetAttribute("PacketSize", UintegerValue(packetSize));
    crossClient.SetAttribute("Interval", TimeValue(Seconds(1.0 / std::max(1u, wirelessPps))));
    clientApps.Add(crossClient.Install(wirelessNodes.Get(1)));

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
    double avgDelayMs = dataRxPkts > 0 ? (delaySumSec / dataRxPkts) * 100.0 : 0.0;
    double pdr = dataTxPkts > 0 ? static_cast<double>(dataRxPkts) / static_cast<double>(dataTxPkts) : 0.0;
    double dropRatio = dataTxPkts > 0 ? static_cast<double>(dataLostPkts) / static_cast<double>(dataTxPkts) : 0.0;

    double energyConsumedJ = 0.0;
    for (uint32_t i = 0; i < energySources.GetN(); ++i)
    {
        Ptr<energy::BasicEnergySource> source = DynamicCast<energy::BasicEnergySource>(energySources.Get(i));
        if (source)
        {
            energyConsumedJ += (100.0 - source->GetRemainingEnergy());
        }
    }

    std::cout << "Topology: wirelessNodes=5, wiredNodes=5, p2pLinks=1" << std::endl;
    std::cout << "P2P gateway link: " << p2pIfaces.GetAddress(0) << " <-> " << p2pIfaces.GetAddress(1) << std::endl;
    std::cout << "Throughput(bps): " << throughputBps << std::endl;
    std::cout << "AvgDelay(ms):    " << avgDelayMs << std::endl;
    std::cout << "PDR:             " << pdr << std::endl;
    std::cout << "DropRatio:       " << dropRatio << std::endl;
    std::cout << "Energy(J):       " << energyConsumedJ << std::endl;

    AppendCsv(csvPath,
              seed,
              run,
              simTime,
              packetSize,
              wirelessSpeed,
              wiredTeacherFlows,
              wiredTeacherPps,
              wirelessTeacherFlows,
              wirelessTeacherPps,
              wiredFlows,
              wiredPps,
              wirelessFlows,
              wirelessPps,
              dataTxPkts,
              dataRxPkts,
              dataLostPkts,
              throughputBps,
              avgDelayMs,
              pdr,
              dropRatio,
              energyConsumedJ);

    Simulator::Destroy();
    return 0;
}
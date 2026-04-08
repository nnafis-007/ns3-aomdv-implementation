#include "ns3/aomdv-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AomdvExample");

int
main(int argc, char* argv[])
{
    uint32_t nNodes = 10;
    double simTime = 30.0;
    double txRange = 250.0;
    uint32_t maxPaths = 3;
    bool enableLog = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nNodes", "Number of nodes", nNodes);
    cmd.AddValue("simTime", "Simulation time (s)", simTime);
    cmd.AddValue("txRange", "Transmission range (m)", txRange);
    cmd.AddValue("maxPaths", "Max alternate AOMDV paths", maxPaths);
    cmd.AddValue("enableLog", "Enable NS_LOG output", enableLog);
    cmd.Parse(argc, argv);

    if (enableLog)
    {
        LogComponentEnable("AomdvRoutingProtocol", LOG_LEVEL_INFO);
        LogComponentEnable("AomdvRoutingTable", LOG_LEVEL_DEBUG);
    }

    NodeContainer nodes;
    nodes.Create(nNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("DsssRate1Mbps"),
                                 "ControlMode",
                                 StringValue("DsssRate1Mbps"));

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(txRange));

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"),
                                  "Y",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"));
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                              "Speed",
                              StringValue("ns3::UniformRandomVariable[Min=1|Max=20]"),
                              "Pause",
                              StringValue("ns3::ConstantRandomVariable[Constant=2]"),
                              "PositionAllocator",
                              StringValue("ns3::RandomRectanglePositionAllocator"));
    mobility.Install(nodes);

    AomdvHelper aomdv;
    aomdv.Set("MaxPaths", UintegerValue(maxPaths));
    aomdv.Set("ActiveRouteTimeout", TimeValue(Seconds(3)));
    aomdv.Set("EnableHello", BooleanValue(true));
    aomdv.Set("HelloInterval", TimeValue(Seconds(1)));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aomdv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    uint16_t port = 9;

    UdpEchoServerHelper server(port);
    ApplicationContainer serverApps = server.Install(nodes.Get(nNodes - 1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));

    UdpEchoClientHelper client(interfaces.GetAddress(nNodes - 1), port);
    client.SetAttribute("MaxPackets", UintegerValue(1000));
    client.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    client.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps = client.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Ptr<OutputStreamWrapper> routingStream =
        Create<OutputStreamWrapper>("aomdv.routes", std::ios::out);
    aomdv.PrintRoutingTableAllAt(Seconds(10), routingStream);

    wifiPhy.EnablePcapAll("aomdv");

    std::cout << "Running AOMDV simulation: " << nNodes << " nodes, " << simTime << "s, " << maxPaths
              << " max paths\n";

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    auto stats = monitor->GetFlowStats();

    double totalPDR = 0;
    double totalDelay = 0;
    double totalTput = 0;
    int flowCount = 0;

    std::cout << "\n========== Flow Statistics ==========\n";
    for (auto& [id, fs] : stats)
    {
        auto t = classifier->FindFlow(id);
        double pdr = (fs.txPackets > 0) ? 100.0 * fs.rxPackets / fs.txPackets : 0.0;
        double delay = (fs.rxPackets > 0) ? fs.delaySum.GetSeconds() / fs.rxPackets * 1000.0 : 0.0;
        double tput = fs.rxBytes * 8.0 / simTime / 1000.0;

        std::cout << "Flow " << id << "  " << t.sourceAddress << " -> " << t.destinationAddress << "\n"
                  << "  Tx Packets : " << fs.txPackets << "\n"
                  << "  Rx Packets : " << fs.rxPackets << "\n"
                  << "  PDR        : " << pdr << " %\n"
                  << "  Mean Delay : " << delay << " ms\n"
                  << "  Throughput : " << tput << " kbps\n\n";

        if (fs.txPackets > 0)
        {
            totalPDR += pdr;
            totalDelay += delay;
            totalTput += tput;
            flowCount++;
        }
    }

    if (flowCount > 0)
    {
        std::cout << "======= Averages across " << flowCount << " flows =======\n"
                  << "PDR       : " << totalPDR / flowCount << " %\n"
                  << "Delay     : " << totalDelay / flowCount << " ms\n"
                  << "Throughput: " << totalTput / flowCount << " kbps\n";
    }

    Simulator::Destroy();
    return 0;
}

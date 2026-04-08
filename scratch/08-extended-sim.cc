#include "ns3/aodv-module.h"
#include "ns3/aomdv-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AodvAomdvComparison");

int
main(int argc, char* argv[])
{
	uint32_t nNodes = 10;
	double simTime = 20.0;
	std::string protocol = "AODV";
	double errorRate = 0.0;
	double minSpeed = 1.0;
	double maxSpeed = 5.0;
	double pauseTime = 2.0;
	uint32_t nFlows = 10;
	uint32_t packetSize = 512;
	double appInterval = 0.12;
	double lossPenaltyMs = -0.2;
	std::string csvFile = "aodv-aomdv-results.csv";
	double maxPosition = 130.0;
	uint32_t seed = 1;
	uint32_t run = 1;
	double initialEnergyJ = 100.0;

	CommandLine cmd;
	cmd.AddValue("nodes", "Number of MANET nodes", nNodes);
	cmd.AddValue("time", "Simulation time (seconds)", simTime);
	cmd.AddValue("protocol", "Routing protocol: AODV, AOMDV", protocol);
	cmd.AddValue("errorRate", "Packet error rate [0.0 - 1.0]", errorRate);
	cmd.AddValue("minSpeed", "Min node speed (m/s)", minSpeed);
	cmd.AddValue("speed", "Max node speed (m/s)", maxSpeed);
	cmd.AddValue("pause", "Pause time between waypoints (s)", pauseTime);
	cmd.AddValue("flows", "Number of UDP traffic flows", nFlows);
	cmd.AddValue("packetSize", "UDP payload size (bytes)", packetSize);
	cmd.AddValue("interval", "Per-flow UDP packet interval (s)", appInterval);
	cmd.AddValue("lossPenaltyMs",
				 "Penalty delay in ms for each lost packet (negative => simTime*1000)",
				 lossPenaltyMs);
	cmd.AddValue("csv", "Output CSV file name", csvFile);
	cmd.AddValue("seed", "RNG seed", seed);
	cmd.AddValue("run", "RNG run index", run);
	cmd.AddValue("area", "Simulation square side length (m)", maxPosition);
	cmd.AddValue("initialEnergyJ", "Initial node energy (J)", initialEnergyJ);
	cmd.Parse(argc, argv);

	if (lossPenaltyMs < 0.0)
	{
		lossPenaltyMs = simTime * 20.0;
	}

	RngSeedManager::SetSeed(seed);
	RngSeedManager::SetRun(run);
	std::srand(seed);

	NodeContainer nodes;
	nodes.Create(nNodes);

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
	NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

	BasicEnergySourceHelper energySource;
	energySource.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(initialEnergyJ));
	energy::EnergySourceContainer sources = energySource.Install(nodes);

	WifiRadioEnergyModelHelper radioEnergy;
	radioEnergy.Install(devices, sources);

	if (errorRate > 0.0)
	{
		for (uint32_t i = 0; i < devices.GetN(); i++)
		{
			Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
			em->SetAttribute("ErrorRate", DoubleValue(errorRate));
			em->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));

			Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(devices.Get(i));
			if (wifiDev)
			{
				wifiDev->GetPhy()->SetAttribute("PostReceptionErrorModel", PointerValue(em));
			}
		}
	}

	InternetStackHelper internet;
	if (protocol == "AODV")
	{
		AodvHelper aodv;
		internet.SetRoutingHelper(aodv);
	}
	else if (protocol == "AOMDV")
	{
		AomdvHelper aomdv;
		internet.SetRoutingHelper(aomdv);
	}
	else
	{
		std::cerr << "Unknown protocol: " << protocol << ". Use AODV or AOMDV.\n";
		return 1;
	}
	internet.Install(nodes);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

	MobilityHelper mobility;
	std::ostringstream speedStr, pauseStr;
	speedStr << "ns3::UniformRandomVariable[Min=" << minSpeed << "|Max=" << maxSpeed << "]";
	pauseStr << "ns3::ConstantRandomVariable[Constant=" << pauseTime << "]";

	Ptr<RandomRectanglePositionAllocator> wpAlloc = CreateObject<RandomRectanglePositionAllocator>();
	wpAlloc->SetX(CreateObjectWithAttributes<UniformRandomVariable>(
		"Min", DoubleValue(0.0), "Max", DoubleValue(maxPosition)));
	wpAlloc->SetY(CreateObjectWithAttributes<UniformRandomVariable>(
		"Min", DoubleValue(0.0), "Max", DoubleValue(maxPosition)));

	mobility.SetPositionAllocator(wpAlloc);
	mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
							  "Speed",
							  StringValue(speedStr.str()),
							  "Pause",
							  StringValue(pauseStr.str()),
							  "PositionAllocator",
							  PointerValue(wpAlloc));
	mobility.Install(nodes);

	uint16_t basePort = 9000;
	ApplicationContainer sinkApps, sourceApps;
	std::set<uint32_t> sinkNodeIds;

	for (uint32_t i = 0; i < nFlows; i++)
	{
		uint32_t src = std::rand() % nNodes;
		uint32_t dst = std::rand() % nNodes;
		while (dst == src)
		{
			dst = std::rand() % nNodes;
		}

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

	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	Simulator::Stop(Seconds(simTime));
	Simulator::Run();

	monitor->CheckForLostPackets();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

	uint64_t dataTx = 0, dataRx = 0, dataLost = 0;
	uint64_t dataTxBytes = 0, dataRxBytes = 0;
	double dataDelaySum = 0.0;
	uint64_t routingTx = 0, routingRx = 0;

	for (const auto& entry : stats)
	{
		Ipv4FlowClassifier::FiveTuple ft = classifier->FindFlow(entry.first);
		const FlowMonitor::FlowStats& fs = entry.second;

		bool isRouting = (ft.destinationPort == 654 || ft.destinationPort == 698);
		if (isRouting)
		{
			routingTx += fs.txPackets;
			routingRx += fs.rxPackets;
		}
		else
		{
			dataTx += fs.txPackets;
			dataRx += fs.rxPackets;
			dataTxBytes += fs.txBytes;
			dataRxBytes += fs.rxBytes;
			dataLost += fs.lostPackets;
			if (fs.rxPackets > 0)
			{
				dataDelaySum += fs.delaySum.GetSeconds();
			}
		}
	}

	double throughputBps = (simTime > 0.0) ? (dataRxBytes * 8.0) / simTime : 0.0;
	double goodputBps = (simTime > 0.0) ? (dataRx * packetSize * 8.0) / simTime : 0.0;
	double throughputKbps = throughputBps / 1000.0;
	double pdr = (dataTx > 0) ? (static_cast<double>(dataRx) / dataTx) * 100.0 : 0.0;
	double avgDelay =
		(dataTx > 0)
			? (((dataDelaySum * 1000.0) + (static_cast<double>(dataLost) * lossPenaltyMs)) /
			   static_cast<double>(dataTx))
			: 0.0;
	double packetLossRate = (dataTx > 0) ? (static_cast<double>(dataLost) / dataTx) * 100.0 : 0.0;

	double sinkPowerConsumptionJ = 0.0;
	for (uint32_t nodeId : sinkNodeIds)
	{
		Ptr<energy::BasicEnergySource> src = DynamicCast<energy::BasicEnergySource>(sources.Get(nodeId));
		if (src)
		{
			sinkPowerConsumptionJ += (initialEnergyJ - src->GetRemainingEnergy());
		}
	}

	std::cout << std::fixed << std::setprecision(2)
			  << "Protocol=" << protocol << " Tx=" << dataTx << " Rx=" << dataRx
			  << " Lost=" << dataLost << " PDR=" << pdr << "%"
			  << " THPT_kbps=" << throughputKbps << " GPT_bps=" << goodputBps
			  << " AvgDelay=" << avgDelay << std::endl;

	bool fileExists = false;
	{
		std::ifstream test(csvFile);
		fileExists = test.good();
	}

	std::ofstream csv(csvFile, std::ios::app);
	if (!fileExists)
	{
		csv << "Protocol,Nodes,Flows,Speed_mps,PauseTime_s,ErrorRate,SimTime_s,PacketSize_B,Seed,Run,"
			<< "DataTxPkts,DataRxPkts,DataLostPkts,RoutingTxPkts,RoutingRxPkts,"
			<< "PDR_pct,THPT_bps,GPT_bps,Throughput_kbps,AvgDelay,PCMP_J,"
			<< "PacketLossRate_pct\n";
	}

	csv << protocol << "," << nNodes << "," << nFlows << "," << maxSpeed << "," << pauseTime << ","
		<< errorRate << "," << simTime << "," << packetSize << "," << seed << "," << run << ","
		<< dataTx << "," << dataRx << "," << dataLost << "," << routingTx << "," << routingRx << ","
		<< std::setprecision(4) << pdr << "," << throughputBps << "," << goodputBps << ","
		<< throughputKbps << "," << avgDelay << ","
		<< sinkPowerConsumptionJ << "," << packetLossRate << "\n";
	csv.close();

	Simulator::Destroy();
	return 0;
}

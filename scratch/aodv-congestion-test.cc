/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AodvCongestionTest");

class AodvCongestionExample : public Object
{
public:
    AodvCongestionExample();
    void Configure(int argc, char **argv);
    void Run();

private:
    void CreateNodes();
    void ConfigureWifi();
    void InstallInternetStack();
    void ConfigurePositions();
    void InstallApplications();
    void SetupFlowMonitor();
    void ProcessFlowMonitor();
    
    // Callbacks
    void ReceivePacket(Ptr<Socket> socket);
    void SendPackets(Ptr<Socket> socket, Ipv4Address destination);
    void TracePackets(std::string context, Ptr<const Packet> packet);
    void TraceCongestion(std::string context, Ipv4Address nodeAddr);
    
    NodeContainer m_nodes;
    NodeContainer m_destinations;
    NetDeviceContainer m_devices;
    Ipv4InterfaceContainer m_interfaces;
    
    uint32_t m_numNodes;          // Regular nodes
    uint32_t m_numDestinations;   // Destination nodes
    uint32_t m_packetSize;        // Bytes per packet
    uint32_t m_numPackets;        // Packets per second
    double m_txRange;             // Transmission range (meters)
    double m_simTime;             // Simulation time (seconds)
    
    Ptr<FlowMonitor> m_flowMonitor;
    FlowMonitorHelper m_flowHelper;
    
    // Statistics tracking
    std::map<uint32_t, uint32_t> m_packetCountPerNode;
    std::map<Ipv4Address, uint32_t> m_congestionEvents;
    std::map<Ipv4Address, Time> m_congestionTimes;
};

AodvCongestionExample::AodvCongestionExample()
    : m_numNodes(25)
    , m_numDestinations(4)
    , m_packetSize(1000)
    , m_numPackets(100)
    , m_txRange(250.0)
    , m_simTime(300.0)
{
}

void
AodvCongestionExample::Configure(int argc, char **argv)
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("numNodes", "Number of nodes", m_numNodes);
    cmd.AddValue("numDestinations", "Number of destinations", m_numDestinations);
    cmd.AddValue("packetSize", "Size of packets", m_packetSize);
    cmd.AddValue("numPackets", "Number of packets", m_numPackets);
    cmd.AddValue("txRange", "Transmission range", m_txRange);
    cmd.AddValue("simTime", "Simulation time", m_simTime);
    cmd.Parse(argc, argv);
}

void
AodvCongestionExample::CreateNodes()
{
    m_nodes.Create(m_numNodes);
    m_destinations.Create(m_numDestinations);
}

void
AodvCongestionExample::ConfigureWifi()
{
    // Configure WiFi PHY and channel
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                  "Exponent", DoubleValue(3.0),
                                  "ReferenceDistance", DoubleValue(1.0),
                                  "ReferenceLoss", DoubleValue(46.6777));

    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("TxPowerStart", DoubleValue(16.0206)); // 40mW
    wifiPhy.Set("TxPowerEnd", DoubleValue(16.0206));
    wifiPhy.Set("RxSensitivity", DoubleValue(-89));
    wifiPhy.Set("CcaEdThreshold", DoubleValue(-92));

    // Configure WiFi MAC
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue("DsssRate1Mbps"),
                                "ControlMode", StringValue("DsssRate1Mbps"));

    m_devices = wifi.Install(wifiPhy, wifiMac, NodeContainer(m_nodes, m_destinations));
}

void
AodvCongestionExample::ConfigurePositions()
{
    MobilityHelper mobility;
    
    // Position nodes in a grid with randomness
    Ptr<RandomBoxPositionAllocator> positionAlloc = 
        CreateObject<RandomBoxPositionAllocator>();
    positionAlloc->SetAttribute("X", 
        StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    positionAlloc->SetAttribute("Y", 
        StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
    positionAlloc->SetAttribute("Z", 
        StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    // Position destinations at corners
    Ptr<ListPositionAllocator> destPositionAlloc = 
        CreateObject<ListPositionAllocator>();
    destPositionAlloc->Add(Vector(0.0, 0.0, 0.0));
    destPositionAlloc->Add(Vector(1000.0, 0.0, 0.0));
    destPositionAlloc->Add(Vector(0.0, 1000.0, 0.0));
    destPositionAlloc->Add(Vector(1000.0, 1000.0, 0.0));
    
    MobilityHelper destMobility;
    destMobility.SetPositionAllocator(destPositionAlloc);
    destMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    destMobility.Install(m_destinations);
}

void
AodvCongestionExample::InstallInternetStack()
{
    AodvHelper aodv;
    // Configure AODV parameters
    aodv.Set("ActiveRouteTimeout", TimeValue(Seconds(3)));
    aodv.Set("CongestionThreshold", UintegerValue(800));
    
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(NodeContainer(m_nodes, m_destinations));
    
    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    m_interfaces = ipv4.Assign(m_devices);
}

void
AodvCongestionExample::InstallApplications()
{
    // Create sending applications at source nodes
    for (uint32_t i = 0; i < m_numNodes; i++)
    {
        Ptr<Socket> source = Socket::CreateSocket(m_nodes.Get(i),
                                                TypeId::LookupByName("ns3::UdpSocketFactory"));
        
        // Randomly select destinations for each source
        uint32_t numDests = rand() % 3 + 1; // 1 to 3 destinations per source
        for (uint32_t j = 0; j < numDests; j++)
        {
            uint32_t destIndex = rand() % m_numDestinations;
            Ipv4Address destAddr = m_interfaces.GetAddress(m_numNodes + destIndex);
            
            // Schedule packet transmission
            Simulator::Schedule(Seconds(10.0 + (rand() % 10)),
                              &AodvCongestionExample::SendPackets,
                              this,
                              source,
                              destAddr);
        }
    }

    // Create receiving applications at destinations
    for (uint32_t i = 0; i < m_numDestinations; i++)
    {
        Ptr<Socket> sink = Socket::CreateSocket(m_destinations.Get(i),
                                              TypeId::LookupByName("ns3::UdpSocketFactory"));
        sink->Bind(InetSocketAddress(m_interfaces.GetAddress(m_numNodes + i), 9));
        sink->SetRecvCallback(MakeCallback(&AodvCongestionExample::ReceivePacket, this));
    }
}

void
AodvCongestionExample::SetupFlowMonitor()
{
    m_flowMonitor = m_flowHelper.InstallAll();
    
    // Add flow monitor probes
    m_flowMonitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    m_flowMonitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    m_flowMonitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));
}

void
AodvCongestionExample::ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        // Update packet count for this node
        uint32_t nodeId = socket->GetNode()->GetId();
        m_packetCountPerNode[nodeId]++;
        
        // Check for congestion
        if (m_packetCountPerNode[nodeId] >= 800) // Congestion threshold
        {
            InetSocketAddress inetFrom = InetSocketAddress::ConvertFrom(from);
            Ipv4Address sourceAddr = inetFrom.GetIpv4();
            
            // Record congestion event
            m_congestionEvents[sourceAddr]++;
            m_congestionTimes[sourceAddr] = Simulator::Now();
            
            // Log congestion
            NS_LOG_INFO("Congestion detected at node " << nodeId 
                       << " from source " << sourceAddr 
                       << " at time " << Simulator::Now().GetSeconds());
        }
    }
}

void
AodvCongestionExample::SendPackets(Ptr<Socket> socket, Ipv4Address destination)
{
    for (uint32_t i = 0; i < m_numPackets; i++)
    {
        Ptr<Packet> packet = Create<Packet>(m_packetSize);
        socket->SendTo(packet, 0, InetSocketAddress(destination, 9));
        
        // Add small delay between packets
        Simulator::Schedule(MicroSeconds(100), &AodvCongestionExample::SendPackets, 
                          this, socket, destination);
    }
}

void
AodvCongestionExample::ProcessFlowMonitor()
{
    m_flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>
        (m_flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = m_flowMonitor->GetFlowStats();

    // Print flow statistics
    std::cout << "\nFlow Statistics:\n";
    for (auto i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                 << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
        std::cout << "  Mean Delay: " << i->second.delaySum.GetSeconds() / 
                    i->second.rxPackets << " seconds\n";
    }

    // Print congestion statistics
    std::cout << "\nCongestion Statistics:\n";
    for (const auto& event : m_congestionEvents)
    {
        std::cout << "Source " << event.first << ":\n";
        std::cout << "  Congestion Events: " << event.second << "\n";
        std::cout << "  Last Congestion: " << 
            m_congestionTimes[event.first].GetSeconds() << " seconds\n";
    }
}

void
AodvCongestionExample::Run()
{
    CreateNodes();
    ConfigureWifi();
    InstallInternetStack();
    ConfigurePositions();
    InstallApplications();
    SetupFlowMonitor();

    // Enable logging
    LogComponentEnable("AodvCongestionTest", LOG_LEVEL_INFO);
    
    // Run simulation
    Simulator::Stop(Seconds(m_simTime));
    Simulator::Run();
    
    // Process results
    ProcessFlowMonitor();
    
    Simulator::Destroy();
}

int main(int argc, char *argv[])
{
    Ptr<AodvCongestionExample> test = CreateObject<AodvCongestionExample>();
    test->Configure(argc, argv);
    test->Run();
    return 0;
}
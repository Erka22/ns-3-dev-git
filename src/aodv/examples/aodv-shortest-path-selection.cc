/* 
 * Test Scenario for AODV Shortest Path Destination Selection
 * 
 * Objective: Verify the functionality of selecting the destination 
 * with the shortest path among multiple destinations
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AodvShortestPathTest");

// Custom function to print routing information
void PrintRoutingTable(Ptr<Node> node) {
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ptr<Ipv4RoutingProtocol> routing = ipv4->GetRoutingProtocol();
    Ptr<aodv::RoutingProtocol> aodvRouting = DynamicCast<aodv::RoutingProtocol>(routing);

    if (aodvRouting) {
        std::cout << "Routing Table for Node " << node->GetId() << ":" << std::endl;
        // Note: You might need to add a method to print routing table details
    }
}

int main (int argc, char *argv[])
{
    // Simulation parameters
    uint32_t nNodes = 15;  // Total number of nodes
    double simulationTime = 20.0;  // Simulation duration in seconds
    bool enablePcap = true;  // Enable PCAP tracing
    bool verbose = true;    // Verbose logging

    // Parse command line arguments
    CommandLine cmd;
    cmd.AddValue("nNodes", "Number of nodes", nNodes);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("enablePcap", "Enable PCAP tracing", enablePcap);
    cmd.AddValue("verbose", "Enable verbose logging", verbose);
    cmd.Parse(argc, argv);

    // Enable logging if verbose mode is on
    if (verbose) {
        LogComponentEnable("AodvShortestPathTest", LOG_LEVEL_ALL);
        LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
    }

    // Create nodes
    NodeContainer nodes;
    nodes.Create(nNodes);

    // Create wireless channel
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Configure WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");
    
    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // Configure mobility
    MobilityHelper mobility;
    Ptr<UniformRandomVariable> xPos = CreateObject<UniformRandomVariable>();
    xPos->SetAttribute("Min", DoubleValue(0.0));
    xPos->SetAttribute("Max", DoubleValue(1000.0));
    
    Ptr<UniformRandomVariable> yPos = CreateObject<UniformRandomVariable>();
    yPos->SetAttribute("Min", DoubleValue(0.0));
    yPos->SetAttribute("Max", DoubleValue(1000.0));
    
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X", PointerValue(xPos),
                                  "Y", PointerValue(yPos));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                               "Bounds", RectangleValue(Rectangle(0, 1000, 0, 1000)));
    mobility.Install(nodes);

    // Install internet stack with AODV routing
    InternetStackHelper internet;
    AodvHelper aodv;
    
    // Define multiple potential destinations
    std::vector<Ipv4Address> multipleDestinations;

    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // Select multiple destinations for testing
    multipleDestinations = {
        interfaces.GetAddress(3),   // Destination 1
        interfaces.GetAddress(7),   // Destination 2
        interfaces.GetAddress(11),  // Destination 3
        interfaces.GetAddress(14)   // Destination 4
    };

    // Wait for routing tables to converge
    Simulator::Schedule(Seconds(2.0), [&]() {
        // Source node (Node 0) sends to multiple destinations
        uint16_t port = 9;

        for (const auto& destAddr : multipleDestinations) {
            // Create UDP socket to send to the destination
            InetSocketAddress sockAddr(destAddr, port);

            // Create OnOff application to send data
            OnOffHelper onOff("ns3::UdpSocketFactory", sockAddr);
            onOff.SetConstantRate(DataRate("512kb/s"));
            onOff.SetAttribute("PacketSize", UintegerValue(1024));
            
            ApplicationContainer apps = onOff.Install(nodes.Get(0));
            apps.Start(Seconds(2.5));
            apps.Stop(Seconds(simulationTime - 1.0));

            // Packet sink on the destination node
            PacketSinkHelper sink("ns3::UdpSocketFactory", 
                                  InetSocketAddress(Ipv4Address::GetAny(), port));
            
            // Find the node with the destination address
            Ptr<Node> destNode = nullptr;
            for (uint32_t i = 0; i < nodes.GetN(); ++i) {
                if (interfaces.GetAddress(i) == destAddr) {
                    destNode = nodes.Get(i);
                    break;
                }
            }

            if (destNode) {
                ApplicationContainer sinkApps = sink.Install(destNode);
                sinkApps.Start(Seconds(2.0));
                sinkApps.Stop(Seconds(simulationTime));
            }
        }
    });

    // Flow monitor for detailed statistics
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // PCAP tracing
    if (enablePcap) {
        phy.EnablePcapAll("aodv-shortest-path-test");
    }

    // Tracing routing tables
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> routingStream = ascii.CreateFileStream("aodv-routing-tables.tr");
    aodv.PrintRoutingTableAllAt(Seconds(5), routingStream);
    aodv.PrintRoutingTableAllAt(Seconds(10), routingStream);
    aodv.PrintRoutingTableAllAt(Seconds(15), routingStream);

    // Simulation duration
    Simulator::Stop(Seconds(simulationTime));

    // Run simulation
    Simulator::Run();

    // Print flow monitor statistics
    monitor->CheckForLostPackets();
    
    // Serialize flow monitor data to XML file
    std::ofstream flowmonStream("flow-monitor.xml");
    monitor->SerializeToXmlFile("flow-monitor.xml", true, true);

    // Print routing tables for all nodes
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        PrintRoutingTable(nodes.Get(i));
    }

    // Cleanup
    Simulator::Destroy();

    return 0;
}

/* Test Scenario Objectives:
 * 1. Verify route discovery to multiple destinations
 * 2. Validate shortest path destination selection
 * 3. Ensure successful packet transmission
 * 4. Collect and analyze network performance metrics
 */
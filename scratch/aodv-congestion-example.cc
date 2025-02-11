/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Example script to test AODV congestion control:
 *
 * ./ns3 run "aodv-congestion-example --verbose=1"
 */

#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AodvCongestionExample");

class AodvCongestionExample 
{
public:
  AodvCongestionExample();
  bool Configure(int argc, char **argv);
  void Run();

private:
  void CreateNodes();
  void CreateDevices();
  void InstallInternetStack();
  void InstallApplications();
  
  void ReceivePacket(Ptr<Socket> socket);
  void SendPackets(Ptr<Socket> socket, Ipv4Address destination);
  
  NodeContainer m_nodes;
  NetDeviceContainer m_devices;
  Ipv4InterfaceContainer m_interfaces;
  
  uint32_t m_packetSize;
  uint32_t m_numPackets;
  uint32_t m_congestionThreshold;
  bool m_verbose;
};

AodvCongestionExample::AodvCongestionExample()
    : m_packetSize(1000)
    , m_numPackets(1000)
    , m_congestionThreshold(800)
    , m_verbose(false)
{
}

bool
AodvCongestionExample::Configure(int argc, char **argv)
{
  CommandLine cmd(__FILE__);
  cmd.AddValue("packetSize", "Size of packets", m_packetSize);
  cmd.AddValue("numPackets", "Number of packets", m_numPackets);
  cmd.AddValue("threshold", "Congestion threshold", m_congestionThreshold);
  cmd.AddValue("verbose", "Enable verbose output", m_verbose);
  cmd.Parse(argc, argv);
  
  return true;
}

void
AodvCongestionExample::Run()
{
  CreateNodes();
  CreateDevices();
  InstallInternetStack();
  InstallApplications();
  
  // Enable trace files
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream("aodv-congestion.tr");
  
  // Enable animation
  AnimationInterface anim("aodv-congestion.xml");
  
  if (m_verbose)
  {
    LogComponentEnable("AodvCongestionExample", LOG_LEVEL_ALL);
    LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  }
  
  Simulator::Stop(Seconds(200.0));
  Simulator::Run();
  Simulator::Destroy();
}

void
AodvCongestionExample::CreateNodes()
{
  m_nodes.Create(5); // Source, 2 destinations, 2 intermediate nodes
  
  // Set positions for visualization
  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                               "MinX", DoubleValue(0.0),
                               "MinY", DoubleValue(0.0),
                               "DeltaX", DoubleValue(100),
                               "DeltaY", DoubleValue(100),
                               "GridWidth", UintegerValue(5),
                               "LayoutType", StringValue("RowFirst"));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(m_nodes);
}

void
AodvCongestionExample::CreateDevices()
{
  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211b);
  
  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
  wifiPhy.SetChannel(wifiChannel.Create());
  
  WifiMacHelper wifiMac;
  wifiMac.SetType("ns3::AdhocWifiMac");
  
  m_devices = wifi.Install(wifiPhy, wifiMac, m_nodes);
}

void
AodvCongestionExample::InstallInternetStack()
{
  AodvHelper aodv;
  // Configure AODV parameters
  Config::SetDefault("ns3::aodv::RoutingProtocol::CongestionThreshold", 
                    UintegerValue(m_congestionThreshold));
  
  InternetStackHelper internet;
  internet.SetRoutingHelper(aodv);
  internet.Install(m_nodes);
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  m_interfaces = ipv4.Assign(m_devices);
}

void
AodvCongestionExample::InstallApplications()
{
  // Create receiving sockets on all nodes
  for (uint32_t i = 0; i < m_nodes.GetN(); i++)
  {
    Ptr<Socket> recvSocket = Socket::CreateSocket(m_nodes.Get(i), 
                                                TypeId::LookupByName("ns3::UdpSocketFactory"));
    recvSocket->Bind(InetSocketAddress(m_interfaces.GetAddress(i), 9));
    recvSocket->SetRecvCallback(MakeCallback(&AodvCongestionExample::ReceivePacket, this));
  }
  
  // Create sending socket at source node (node 0)
  Ptr<Socket> source = Socket::CreateSocket(m_nodes.Get(0), 
                                          TypeId::LookupByName("ns3::UdpSocketFactory"));
  source->SetAllowBroadcast(true);
  
  // Schedule transmissions to multiple destinations
  Simulator::Schedule(Seconds(1.0), 
                     &AodvCongestionExample::SendPackets, 
                     this, 
                     source, 
                     m_interfaces.GetAddress(1));
  
  Simulator::Schedule(Seconds(50.0), 
                     &AodvCongestionExample::SendPackets, 
                     this, 
                     source, 
                     m_interfaces.GetAddress(2));
}

void
AodvCongestionExample::ReceivePacket(Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom(from)))
  {
    if (m_verbose)
    {
      NS_LOG_INFO("Received packet size: " << packet->GetSize());
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
    
    if (m_verbose && (i % 100 == 0))
    {
      NS_LOG_INFO("Sending packet " << i << " to " << destination);
    }
  }
}

int main(int argc, char *argv[])
{
  AodvCongestionExample example;
  if (!example.Configure(argc, argv))
  {
    NS_FATAL_ERROR("Configuration failed!");
    return 1;
  }
  
  example.Run();
  return 0;
}

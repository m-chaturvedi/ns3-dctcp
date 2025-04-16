/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <iomanip>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("StarTcpDctcpComparison");

std::ofstream t1QueueLength;

void
CheckQueueSize(Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetNPackets();
  double linkRate = 1e9; // 1 Gbps
  Time backlog = Seconds(static_cast<double>(qSize * 1500 * 8) / linkRate);

  t1QueueLength << std::fixed << std::setprecision(2)
                << Simulator::Now().GetSeconds() << " "
                << qSize << " "
                << backlog.GetMicroSeconds() << std::endl;

  Simulator::Schedule(MilliSeconds(10), &CheckQueueSize, queue);
}

int
main(int argc, char* argv[])
{
  bool useDctcp = false;
  std::string output = "01-tcp-result-length.dat";

  CommandLine cmd(__FILE__);
  cmd.AddValue("useDctcp", "Use DCTCP instead of TCP", useDctcp);
  cmd.AddValue("output", "Queue length output file", output);
  cmd.Parse(argc, argv);

  t1QueueLength.open(output, std::ios::out);
  t1QueueLength << "#Time(s) qlen(pkts) qlen(us)\n";

  if (useDctcp) {
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpDctcp"));
    Config::SetDefault("ns3::RedQueueDisc::UseEcn", BooleanValue(true));
    Config::SetDefault("ns3::RedQueueDisc::QW", DoubleValue(1));
    Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(5));
    Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(15));
  } else {
    Config::SetDefault("ns3::RedQueueDisc::UseEcn", BooleanValue(false));
    Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(100));
    Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(300));
  }

  Config::SetDefault("ns3::RedQueueDisc::UseHardDrop", BooleanValue(false));
  Config::SetDefault("ns3::RedQueueDisc::MeanPktSize", UintegerValue(1500));
  Config::SetDefault("ns3::RedQueueDisc::MaxSize", QueueSizeValue(QueueSize("477p")));
  Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(1448));

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10us"));

  uint32_t nSpokes = 3;
  PointToPointStarHelper star(nSpokes, p2p);

  InternetStackHelper stack;
  star.InstallStack(stack);
  star.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"));

  uint16_t port = 50000;
  Address sinkAddr(InetSocketAddress(star.GetSpokeIpv4Address(2), port + 3));
  PacketSinkHelper sink("ns3::TcpSocketFactory", sinkAddr);
  ApplicationContainer sinkApp = sink.Install(star.GetSpokeNode(2));

  BulkSendHelper source1("ns3::TcpSocketFactory", sinkAddr);
  source1.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer source1App = source1.Install(star.GetSpokeNode(0));

  BulkSendHelper source2("ns3::TcpSocketFactory", sinkAddr);
  source2.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer source2App = source2.Install(star.GetSpokeNode(1));

  sinkApp.Start(Seconds(1.0));
  sinkApp.Stop(Seconds(10.0));
  source1App.Start(Seconds(1.0));
  source1App.Stop(Seconds(10.0));
  source2App.Start(Seconds(1.0));
  source2App.Stop(Seconds(10.0));

  TrafficControlHelper tchRed;
  tchRed.SetRootQueueDisc("ns3::RedQueueDisc",
                          "LinkBandwidth", StringValue("1Gbps"),
                          "LinkDelay", StringValue("10us"));

  Ptr<Node> hubNode = star.GetHub();
  Ptr<NetDevice> netDev = hubNode->GetDevice(2);

  tchRed.Uninstall(netDev);
  QueueDiscContainer qdiscs = tchRed.Install(netDev);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  Simulator::Schedule(Seconds(2.0), &CheckQueueSize, qdiscs.Get(0));
  Simulator::Stop(Seconds(10.0));
  Simulator::Run();
  Simulator::Destroy();

  t1QueueLength.close();
  return 0;
}
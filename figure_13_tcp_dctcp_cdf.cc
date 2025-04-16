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
#include <fstream>
#include <vector>
#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpDctcpQueueCdf");

std::ofstream cdfFile;
std::vector<uint32_t> queueSamples;

void
CheckQueueSize(Ptr<QueueDisc> queue)
{
    uint32_t qSize = queue->GetNPackets();
    queueSamples.push_back(qSize);
    Simulator::Schedule(MilliSeconds(125), &CheckQueueSize, queue); // Sample every 125ms
}

void
ComputeCdf(const std::vector<uint32_t>& samples, const std::string& label)
{
    std::vector<uint32_t> sorted = samples;
    std::sort(sorted.begin(), sorted.end());

    cdfFile << "#Queue_length_(packets) CDF_for_" << label << "\n";
    for (size_t i = 0; i < sorted.size(); i++) {
        double cdf = (i + 1.0) / sorted.size();
        cdfFile << sorted[i] << " " << cdf << "\n";
    }
}

int
main(int argc, char* argv[])
{
    bool useDctcp = false;
    uint32_t numFlows = 2; // Default to 2 flows
    std::string output = "queue-cdf.dat";

    CommandLine cmd(__FILE__);
    cmd.AddValue("useDctcp", "Use DCTCP instead of TCP", useDctcp);
    cmd.AddValue("numFlows", "Number of flows (2 or 20)", numFlows);
    cmd.AddValue("output", "Output filename", output);
    cmd.Parse(argc, argv);

    cdfFile.open(output, std::ios::out);
    // cdfFile << "# Queue length CDF data for Figure 13\n";

    // Configuration based on DCTCP paper
    if (useDctcp) {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpDctcp"));
        Config::SetDefault("ns3::RedQueueDisc::UseEcn", BooleanValue(true));
        Config::SetDefault("ns3::RedQueueDisc::QW", DoubleValue(1)); // Instantaneous queue
        Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(20)); // K=20 for DCTCP
        Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(20)); // Step marking
    } else {
        Config::SetDefault("ns3::RedQueueDisc::UseEcn", BooleanValue(true));
        Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(100)); // High thresholds
        Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(300)); // for TCP
    }

    Config::SetDefault("ns3::RedQueueDisc::UseHardDrop", BooleanValue(false));
    Config::SetDefault("ns3::RedQueueDisc::MeanPktSize", UintegerValue(1500));
    Config::SetDefault("ns3::RedQueueDisc::MaxSize", QueueSizeValue(QueueSize("160p"))); // ~240KB
    Config::SetDefault("ns3::BulkSendApplication::SendSize", UintegerValue(1448));

    // Set up topology
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("50us")); // 100us RTT total
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("1p")));

    // Create star topology with 1 hub and N spokes (sources)
    uint32_t nSpokes = numFlows + 1; // numFlows sources + 1 sink
    PointToPointStarHelper star(nSpokes, p2p);

    InternetStackHelper stack;
    star.InstallStack(stack);
    star.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"));

    // Install applications
    uint16_t port = 50000;
    Address sinkAddr(InetSocketAddress(star.GetSpokeIpv4Address(nSpokes-1), port));
    PacketSinkHelper sink("ns3::TcpSocketFactory", sinkAddr);
    ApplicationContainer sinkApp = sink.Install(star.GetSpokeNode(nSpokes-1));

    ApplicationContainer sourceApps;
    for (uint32_t i = 0; i < numFlows; i++) {
        BulkSendHelper source("ns3::TcpSocketFactory", sinkAddr);
        source.SetAttribute("MaxBytes", UintegerValue(0)); // Unlimited transfer
        sourceApps.Add(source.Install(star.GetSpokeNode(i)));
    }

    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(10.0));
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(10.0));

    // Install queue discipline on hub's incoming devices
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::RedQueueDisc",
                       "LinkBandwidth", StringValue("1Gbps"),
                       "LinkDelay", StringValue("50us"));

    Ptr<Node> hubNode = star.GetHub();
    Ptr<NetDevice> netDev = hubNode->GetDevice(nSpokes-1); // Last device is bottleneck

    tch.Uninstall(netDev);
    QueueDiscContainer qdiscs = tch.Install(netDev);

    // Start queue monitoring after 1 second (let flows establish)
    Simulator::Schedule(Seconds(2.0), &CheckQueueSize, qdiscs.Get(0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Compute and output CDF
    std::string label = (useDctcp ? "DCTCP" : "TCP") + std::string("_") +
                       std::to_string(numFlows) + "_flows";
    ComputeCdf(queueSamples, label);

    Simulator::Destroy();
    cdfFile.close();
    return 0;
}

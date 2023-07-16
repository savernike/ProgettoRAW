#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

// additions to enable flowMonitor
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

//addition to enable printing in file
#include <fstream>



// Default Network Topology
//
//   Wifi 10.1.3.0 + 2 p2p link towards a remote server

//
//       10.1.1.0         10.1.2.0
// n0(AP) -------------- n1---------------n2(server)
//    point-to-point1    point-to-point2
//

//Evaluation of metrics when changing the IEEE 802.11 PHY layer
//(11g/11n...)


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("progetto");



int 
main (int argc, char *argv[])
{
  bool verbose = false;
  uint32_t nWifi = 8;
  bool tracing = true; //serve per abilitare la segnalazione ma qui non serve perche useremo flow monitor
 

  uint32_t flows = 1; //nel nostro caso sarà parametrico

  int seed=1; //anche seed sarà parametrico

  int endSim=10;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "Tell applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("flows", "Number of cbr flows ", flows);
  cmd.AddValue ("seed", "seed of the simulation", seed);
  cmd.AddValue ("nWifi", "Number of wifiSTA", nWifi);
  
  cmd.Parse (argc,argv);

  SeedManager::SetSeed (seed);



  if(flows >nWifi)
  {
	  std::cout << "the number of flows should be minor or equal to the number of STA that is 8" <<std::endl;
	  return 1;
  }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    }

//******link 1************
  NodeContainer p2pNodes1; //collegamento n0(sevrer) e n1(nodo)
  p2pNodes1.Create (2);

  //creazione del canale (link) per la prima sottorete
  PointToPointHelper pointToPoint1; //parametri sul primo link
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("200Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("1ms"));

  //creazione scheda di rete
  NetDeviceContainer p2pDevices1;//interfaccia 1
  p2pDevices1 = pointToPoint1.Install (p2pNodes1);//interfaccia settata sui nodi
  
//*******fine link 1*******  

  
//*****link 2***********
  NodeContainer p2pNodes2; //collegamento n1(nodo) e n2(AP)
  p2pNodes2.Add (p2pNodes1.Get(1));
  p2pNodes2.Create (1); //creo il nodo
  
  
  
  //creazione del canale (link) per la seconda sottorete
  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("300Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("1ms"));
  
  //creazione scheda di rete per la seconda sottorete
  NetDeviceContainer p2pDevices2;
  p2pDevices2 = pointToPoint2.Install (p2pNodes2);
//*****fine link 2*****


//Wireless settings --le stazioni devono essere dichiarate da 0 mentre l'AP esiste già
  NodeContainer wifiStaNodes; //Container delle stazioni
  wifiStaNodes.Create (nWifi); //nWiFi è il numero di stazioni e la variabile è scritta alla riga 37 

  NodeContainer wifiApNode = p2pNodes2.Get (1); //Container dell'AP con le stazioni



  WifiHelper wifi;

  //check wifi-standard.h for other standards
  wifi.SetStandard(WIFI_STANDARD_80211g); //si cambia qui lo standard quando si deve fare confrontro tra due standard


  YansWifiChannelHelper channel = YansWifiChannelHelper::Default (); // di default c'è la logdistance
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
    //  The default is a channel model with a propagation delay equal to a constant, the speed of light,
  //and a propagation loss based on a log distance model with a reference loss of 46.6777 dB at reference distance of 1m.


  wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager"); //questo è algoritmo che serve per il controllo del rate (si usa dove c'è 11g e 11n)

  WifiMacHelper mac; //la descrizionde della parte macc è sempre la stessa indifferentemente dalla tipologia
  Ssid ssid = Ssid ("my-WIFI");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);



  //*************************************************************************

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (5.0),
                                 "GridWidth", UintegerValue (5), //GridWidth: The number of objects laid out on a line (significa che su una riga saranno al massimo 4)
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);


  //AP in the center of the topology
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(10, 2.5, 0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiApNode);

//livello rete
  InternetStackHelper stack; //installiamo internet nei nodi
  stack.Install (wifiStaNodes); //tra l'AP e le sta
  stack.Install(p2pNodes1); //link 1 tra server e nodo
  stack.Install(p2pNodes2.Get(1)); //AP(n2)

  Ipv4AddressHelper address;
  address.SetBase ("10.1.3.0", "255.255.255.0"); //avremo una base di indirizzi per rete wifi e altra base di indirizzi per i collegamenti p2p
  
  address.Assign (apDevices);
  Ipv4InterfaceContainer interfaces;
  interfaces = address.Assign (staDevices);


  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address.Assign (p2pDevices1);
  
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces2;
  p2pInterfaces2 = address.Assign (p2pDevices2);  

  
  //Applications begin **************************************************************************************

  	  std::cout<<"Simulating "<< flows<< " cbr flows"<<std::endl;

  	  for (uint32_t i=0; i<flows; i++)
  	  {
  		  uint16_t port = 9;
  		  PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  		  ApplicationContainer apps_sink = sink.Install (wifiStaNodes.Get (i));


  		  Ipv4Address sinkAddress = interfaces.GetAddress (i);
  		  static const uint32_t packetSize = 1420;
  		  OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (sinkAddress, port));
  		  onoff.SetConstantRate (DataRate ("12Mb/s"), packetSize);
  		  onoff.SetAttribute ("StartTime", TimeValue (Seconds (1.0)));
  		  onoff.SetAttribute ("StopTime", TimeValue (Seconds (endSim)));
  		  ApplicationContainer apps_source = onoff.Install (p2pNodes1.Get (0)); //sending pkts

  		  apps_source.Start(Seconds(1.0));
  		  apps_source.Stop(Seconds(endSim));

  		   std::cout<<"Created flow "<< i <<" to STA "<<wifiStaNodes.Get (i)->GetId()<<std::endl;
  	 }

    //Applications end **************************************************************************************

  	 Ipv4GlobalRoutingHelper::PopulateRoutingTables (); //quando si hanno piu sottoreti si deve usare un protocollo per popolare i nodi


  	  // AAA Install FlowMonitor on all nodes before Stopping
  	  FlowMonitorHelper flowmon;
  	  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds(endSim));
  
  if (tracing == true)
      {
  	  AsciiTraceHelper ascii;
  	  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("progetto4.tr");
  	  phy.EnableAsciiAll (stream);

      }

  
  Simulator::Run ();
  //Statistics to print and plot
    double avg_throughput = 0;
    double avg_rx_pkts = 0;
    double avg_delay = 0;

    double avg_lost_ratio = 0;

    // Print per flow statistics
      monitor->CheckForLostPackets ();
      Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
      FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
      for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
      {
         Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
         std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
         std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
         std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
         std::cout << "  TxThroughp: " << i->second.txBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) / 1000 /1000  << " Mbps\n";
         std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
         std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
         std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) / 1000 /1000  << " Mbps\n";
         std::cout << "  Mean delay: " << i->second.delaySum.GetSeconds () / i->second.rxPackets << " s\n";

         double throughput= i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) / 1000 /1000;
         double delay = i->second.delaySum.GetSeconds () / i->second.rxPackets;

         double rx = i->second.rxPackets;
         double tx = i->second.txPackets;

         double lost_ratio = 1 - (rx/tx);

         avg_throughput = avg_throughput + throughput;
         avg_rx_pkts = avg_rx_pkts + i->second.rxPackets;
         avg_delay = avg_delay + delay;

         avg_lost_ratio = avg_lost_ratio + lost_ratio;

     }

     	std::ofstream myfile; //open file without overwrite previous stats. ci portiamo i risultati su file 
     	myfile.open ("avg_throughput.txt", std::ios_base::app);
     	myfile <<avg_throughput/flows<<"\n";
     	myfile.close();

     	myfile.open ("avg_rx_pkts.txt",  std::ios_base::app);
     	myfile << avg_rx_pkts/flows << "\n";
     	myfile.close();

     	myfile.open ("avg_delay.txt",  std::ios_base::app);
     	myfile << avg_delay/flows << "\n";
     	myfile.close();

     	myfile.open ("avg_lost_ratio.txt",  std::ios_base::app);
     	myfile << avg_lost_ratio/flows *100 << "\n"; //value expressed in percentage
     	myfile.close();



    Simulator::Destroy ();
    return 0;
 }

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
// n0(server) -------------- n1---------------n2(AP)
//    point-to-point1    point-to-point2
//

//Evaluation of metrics when changing the IEEE 802.11 PHY layer
//(11g/11n...)


using namespace ns3;//viene inserita sempre perchè accediamo a tutte le funzioni di ns-3

NS_LOG_COMPONENT_DEFINE ("progetto");

int 
main (int argc, char *argv[])
{
  bool verbose = false;
  uint32_t nWifi = 8;
  bool tracing = true; //serve per abilitare la segnalazione 

  uint32_t flows = 2; //nel nostro caso sarà parametrico

  int seed=1; //anche seed sarà parametrico

  int endSim=10;

  CommandLine cmd (__FILE__);//per modificare da command line la variabile
  cmd.AddValue ("verbose", "Tell applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("flows", "Number of cbr flows ", flows);
  cmd.AddValue ("seed", "seed of the simulation", seed);

  cmd.Parse (argc,argv);

  SeedManager::SetSeed (seed);



  if(flows >nWifi)
  {
	  std::cout << "the number of flows should be minor or equal to the number of STA that is 8" <<std::endl;
	  return 1;
  }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);//abilitiamo il log per vedere i pacchetti che si scambiano le app
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

  //creazione scheda di rete controllata da SO che gestisce i protocolli per trasmettere sul canale
  NetDeviceContainer p2pDevices1;//interfaccia 1
  p2pDevices1 = pointToPoint1.Install (p2pNodes1);//il link punto punto va associato alla scheda di rete ma applicato ai nodi

//*****link 2***********
  NodeContainer p2pNodes2; //collegamento n1(nodo) e n2(AP)
  p2pNodes2.Add (p2pNodes1.Get(1));
  p2pNodes2.Create (1); //creo il nodo
  
  //creazione del canale (link) per la seconda sottorete
  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("300Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("1ms"));
  
  //creazione scheda di rete per la seconda sottorete
  NetDeviceContainer p2pDevices2;//interfaccia 2
  p2pDevices2 = pointToPoint2.Install (p2pNodes2);


//Wireless settings --le stazioni devono essere dichiarate da 0 mentre l'AP esiste già
  NodeContainer wifiStaNodes; //Container delle stazioni
  wifiStaNodes.Create (nWifi); //nWiFi è il numero di stazioni e la variabile è scritta alla riga 37 

  NodeContainer wifiApNode = p2pNodes2.Get (1); //Container dell'AP con le stazioni



  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211g); 


  YansWifiChannelHelper channel = YansWifiChannelHelper::Default (); // di default: logdistance
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
    //  The default is a channel model with a propagation delay equal to a constant, the speed of light,
  //and a propagation loss based on a log distance model with a reference loss of 46.6777 dB at reference distance of 1m.


  wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager"); //questo è algoritmo che serve per il controllo del rate
  
 //creiamo il livello MAC sia per STA che AP in quato abbiamo 2 tipologie diverse di MAC in quanto AP ha più funzioni rispetto alle STAs
  WifiMacHelper mac; //la descrizionde della parte MAC è sempre la stessa indifferentemente dalla tipologia
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

  MobilityHelper mobility;//definisco i nodi sulla griglia

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0), //minimo valore dell'asse x da cui partono le stazioni
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (5.0),
                                 "GridWidth", UintegerValue (5), //su una riga saranno al massimo 5 STAs
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); //nodi fermi
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
  address.SetBase ("10.1.3.0", "255.255.255.0");//indirizzo e maschera vengono assegnati in maniera automatica incrementando l'indirizzo per tutti i nodi partendo dal primo nodo 0 che avrà l'indirizzo appena dichiarato 
  
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

	  std::cout<<"Simulating "<< flows<< " cbr flows"<<std::endl; //flusso CBR

		  uint16_t port = 9; //porta sulla quale il sink raccoglie i pacchetti provenuenti da qualsiasi sorgente
		  PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
		  ApplicationContainer apps_sink = sink.Install (wifiStaNodes.Get (0)); //associamo l'applicazione al server


		  Ipv4Address sinkAddress = interfaces.GetAddress (0);
		  static const uint32_t packetSize = 1420;
		  OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (sinkAddress, port));
		  onoff.SetConstantRate (DataRate ("10Mb/s"), packetSize);
		  onoff.SetAttribute ("StartTime", TimeValue (Seconds (1.0)));
		  onoff.SetAttribute ("StopTime", TimeValue (Seconds (endSim)));
		  ApplicationContainer apps_source = onoff.Install (p2pNodes1.Get (0)); //sending pkts (onoff perchè è un'applicazione che trasmette dati alla quale possiamo dare un periofo di on o off)

		  apps_source.Start(Seconds(1.0));
		  apps_source.Stop(Seconds(endSim));
		  
		  uint16_t port2 = 10;
		  PacketSinkHelper sink2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port2));
		  ApplicationContainer apps_sink2 = sink2.Install (wifiStaNodes.Get (5));


		  Ipv4Address sinkAddress2 = interfaces.GetAddress (5);
		  OnOffHelper onoff2 ("ns3::UdpSocketFactory", InetSocketAddress (sinkAddress2, port2));
		  onoff2.SetConstantRate (DataRate ("10Mb/s"), packetSize);
		  onoff2.SetAttribute ("StartTime", TimeValue (Seconds (1.0)));
		  onoff2.SetAttribute ("StopTime", TimeValue (Seconds (endSim)));
		  ApplicationContainer apps_source2 = onoff2.Install (p2pNodes1.Get (0)); //sending pkts

		  apps_source2.Start(Seconds(1.0));
		  apps_source2.Stop(Seconds(endSim));

  //Applications end **************************************************************************************


  Ipv4GlobalRoutingHelper::PopulateRoutingTables (); //quando si hanno piu sottoreti si deve usare un protocollo per popolare i nodi


  // AAA Install FlowMonitor on all nodes before Stopping
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();



  Simulator::Stop (Seconds(endSim));

  if (tracing == true)
    {

	  phy.EnablePcap ("progetto", staDevices.Get (1), true); //Tracing on the first STA node in promiscuous mode:la stazione si mette in ascolto su tutto quello che viaggia sul canale

    }

  Simulator::Run ();
  
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


    }

   Simulator::Destroy ();//si conclude la simulazione e per fare il cleaning
   return 0;
}

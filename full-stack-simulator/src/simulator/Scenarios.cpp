#include "simulator/Scenarios.h"

#include <iostream>
#include <memory>

#include "analysis/DomainAnalyzer.h"
#include "app/Services.h"
#include "core/Logger.h"
#include "devices/Bridge.h"
#include "devices/Host.h"
#include "devices/Hub.h"
#include "devices/Router.h"
#include "devices/Switch.h"
#include "physical/Link.h"

namespace {

void connect(Link& link, Port* left, Port* right) {
    link.connect(left, right);
}

void printDomains(const DomainReport& report) {
    Logger::data("REPORT", "collision domains", std::to_string(report.collisionDomains));
    Logger::data("REPORT", "broadcast domains", std::to_string(report.broadcastDomains));
}

Host makeHost(const std::string& name, const std::string& mac, const std::string& ip) {
    Host host(name);
    host.addPort("eth0", mac, ip);
    return host;
}

}  // namespace

void Scenarios::showMenu() {
    Logger::section("Full Stack Network Simulator");
    std::cout << "1. Point-to-point physical delivery\n";
    std::cout << "2. Hub star topology\n";
    std::cout << "3. Switch star topology with protocols\n";
    std::cout << "4. Two hub stars connected through a switch\n";
    std::cout << "5. Full stack routed application demo\n";
    std::cout << "6. Run every scenario\n";
    std::cout << "0. Exit\n";
}

bool Scenarios::runChoice(int choice) {
    switch (choice) {
        case 1:
            pointToPoint();
            return true;
        case 2:
            hubStar();
            return true;
        case 3:
            switchStar();
            return true;
        case 4:
            hybridHubSwitch();
            return true;
        case 5:
            fullStackRouted();
            return true;
        case 6:
            runAll();
            return true;
        case 0:
            return false;
        default:
            Logger::warn("SYSTEM", "Invalid menu choice.");
            return true;
    }
}

void Scenarios::pointToPoint() {
    Logger::section("Scenario 1: Point-to-Point");

    Host pc1 = makeHost("PC1", "00:00:00:00:01:01", "10.0.0.1");
    Host pc2 = makeHost("PC2", "00:00:00:00:01:02", "10.0.0.2");
    pc2.installService(makeEchoService());

    Link cable("Link-PC1-PC2");
    connect(cable, pc1.portByName("eth0"), pc2.portByName("eth0"));

    pc1.sendApplication("echo", "10.0.0.2", 7, "hello over dedicated link");
    printDomains(DomainAnalyzer::pointToPoint());
}

void Scenarios::hubStar() {
    Logger::section("Scenario 2: Hub Star");

    Hub hub("Hub1");
    auto* hubP1 = hub.addPort("p1", "10:00:00:00:00:01");
    auto* hubP2 = hub.addPort("p2", "10:00:00:00:00:02");
    auto* hubP3 = hub.addPort("p3", "10:00:00:00:00:03");
    auto* hubP4 = hub.addPort("p4", "10:00:00:00:00:04");
    auto* hubP5 = hub.addPort("p5", "10:00:00:00:00:05");

    Host pc1 = makeHost("PC1", "00:00:00:00:02:01", "10.0.1.1");
    Host pc2 = makeHost("PC2", "00:00:00:00:02:02", "10.0.1.2");
    Host pc3 = makeHost("PC3", "00:00:00:00:02:03", "10.0.1.3");
    Host pc4 = makeHost("PC4", "00:00:00:00:02:04", "10.0.1.4");
    Host pc5 = makeHost("PC5", "00:00:00:00:02:05", "10.0.1.5");

    pc3.installService(makeEchoService());

    Link l1("L1");
    Link l2("L2");
    Link l3("L3");
    Link l4("L4");
    Link l5("L5");
    connect(l1, pc1.portByName("eth0"), hubP1);
    connect(l2, pc2.portByName("eth0"), hubP2);
    connect(l3, pc3.portByName("eth0"), hubP3);
    connect(l4, pc4.portByName("eth0"), hubP4);
    connect(l5, pc5.portByName("eth0"), hubP5);

    pc1.sendApplication("echo", "10.0.1.3", 7, "hub based communication");
    printDomains(DomainAnalyzer::hubStar());
}

void Scenarios::switchStar() {
    Logger::section("Scenario 3: Switch Star");

    Switch sw("Switch1");
    auto* p1 = sw.addPort("p1", "20:00:00:00:00:01");
    auto* p2 = sw.addPort("p2", "20:00:00:00:00:02");
    auto* p3 = sw.addPort("p3", "20:00:00:00:00:03");
    auto* p4 = sw.addPort("p4", "20:00:00:00:00:04");
    auto* p5 = sw.addPort("p5", "20:00:00:00:00:05");

    Host pc1 = makeHost("PC1", "00:00:00:00:03:01", "10.0.2.1");
    Host pc2 = makeHost("PC2", "00:00:00:00:03:02", "10.0.2.2");
    Host pc3 = makeHost("PC3", "00:00:00:00:03:03", "10.0.2.3");
    Host pc4 = makeHost("PC4", "00:00:00:00:03:04", "10.0.2.4");
    Host pc5 = makeHost("PC5", "00:00:00:00:03:05", "10.0.2.5");

    pc3.installService(makeEchoService());
    pc5.installService(makeFileService());

    Link l1("S1");
    Link l2("S2");
    Link l3("S3");
    Link l4("S4");
    Link l5("S5");
    connect(l1, pc1.portByName("eth0"), p1);
    connect(l2, pc2.portByName("eth0"), p2);
    connect(l3, pc3.portByName("eth0"), p3);
    connect(l4, pc4.portByName("eth0"), p4);
    connect(l5, pc5.portByName("eth0"), p5);

    pc1.sendApplication("echo", "10.0.2.3", 7, "switch learns addresses");
    pc2.sendApplication("file", "10.0.2.5", 21, "lab-spec.txt");
    sw.printMacTable();
    printDomains(DomainAnalyzer::switchStar(5));
}

void Scenarios::hybridHubSwitch() {
    Logger::section("Scenario 4: Two Hubs Through One Switch");

    Switch sw("CoreSwitch");
    auto* swLeft = sw.addPort("left", "30:00:00:00:00:01");
    auto* swRight = sw.addPort("right", "30:00:00:00:00:02");

    Hub hubA("HubA");
    auto* hubAUp = hubA.addPort("up", "31:00:00:00:00:01");
    auto* hubA1 = hubA.addPort("a1", "31:00:00:00:00:02");
    auto* hubA2 = hubA.addPort("a2", "31:00:00:00:00:03");
    auto* hubA3 = hubA.addPort("a3", "31:00:00:00:00:04");
    auto* hubA4 = hubA.addPort("a4", "31:00:00:00:00:05");
    auto* hubA5 = hubA.addPort("a5", "31:00:00:00:00:06");

    Hub hubB("HubB");
    auto* hubBUp = hubB.addPort("up", "32:00:00:00:00:01");
    auto* hubB1 = hubB.addPort("b1", "32:00:00:00:00:02");
    auto* hubB2 = hubB.addPort("b2", "32:00:00:00:00:03");
    auto* hubB3 = hubB.addPort("b3", "32:00:00:00:00:04");
    auto* hubB4 = hubB.addPort("b4", "32:00:00:00:00:05");
    auto* hubB5 = hubB.addPort("b5", "32:00:00:00:00:06");

    Host a1 = makeHost("A1", "00:00:00:00:04:01", "10.0.3.1");
    Host a2 = makeHost("A2", "00:00:00:00:04:02", "10.0.3.2");
    Host a3 = makeHost("A3", "00:00:00:00:04:03", "10.0.3.3");
    Host a4 = makeHost("A4", "00:00:00:00:04:04", "10.0.3.4");
    Host a5 = makeHost("A5", "00:00:00:00:04:05", "10.0.3.5");
    Host b1 = makeHost("B1", "00:00:00:00:05:01", "10.0.3.6");
    Host b2 = makeHost("B2", "00:00:00:00:05:02", "10.0.3.7");
    Host b3 = makeHost("B3", "00:00:00:00:05:03", "10.0.3.8");
    Host b4 = makeHost("B4", "00:00:00:00:05:04", "10.0.3.9");
    Host b5 = makeHost("B5", "00:00:00:00:05:05", "10.0.3.10");

    b1.installService(makeEchoService());
    b2.installService(makeEchoService());
    b3.installService(makeEchoService());
    b4.installService(makeEchoService());
    b5.installService(makeEchoService());

    Link hubALink("HubA-SW");
    Link hubBLink("HubB-SW");
    connect(hubALink, hubAUp, swLeft);
    connect(hubBLink, hubBUp, swRight);

    Link aL1("A-L1");
    Link aL2("A-L2");
    Link aL3("A-L3");
    Link aL4("A-L4");
    Link aL5("A-L5");
    Link bL1("B-L1");
    Link bL2("B-L2");
    Link bL3("B-L3");
    Link bL4("B-L4");
    Link bL5("B-L5");
    connect(aL1, a1.portByName("eth0"), hubA1);
    connect(aL2, a2.portByName("eth0"), hubA2);
    connect(aL3, a3.portByName("eth0"), hubA3);
    connect(aL4, a4.portByName("eth0"), hubA4);
    connect(aL5, a5.portByName("eth0"), hubA5);
    connect(bL1, b1.portByName("eth0"), hubB1);
    connect(bL2, b2.portByName("eth0"), hubB2);
    connect(bL3, b3.portByName("eth0"), hubB3);
    connect(bL4, b4.portByName("eth0"), hubB4);
    connect(bL5, b5.portByName("eth0"), hubB5);

    a1.sendApplication("echo", "10.0.3.6", 7, "A1 to B1");
    a2.sendApplication("echo", "10.0.3.7", 7, "A2 to B2");
    a3.sendApplication("echo", "10.0.3.8", 7, "A3 to B3");
    a4.sendApplication("echo", "10.0.3.9", 7, "A4 to B4");
    a5.sendApplication("echo", "10.0.3.10", 7, "A5 to B5");
    sw.printMacTable();
    printDomains(DomainAnalyzer::hybridHubSwitch());
}

void Scenarios::fullStackRouted() {
    Logger::section("Scenario 5: Full Stack Routed Demo");

    Switch leftSwitch("LeftSwitch");
    auto* ls1 = leftSwitch.addPort("h1", "40:00:00:00:00:01");
    auto* ls2 = leftSwitch.addPort("bridge", "40:00:00:00:00:02");

    Bridge bridge("Bridge1");
    auto* br1 = bridge.addPort("left", "41:00:00:00:00:01");
    auto* br2 = bridge.addPort("right", "41:00:00:00:00:02");

    Router routerA("R1");
    auto* rA0 = routerA.addPort("eth0", "42:00:00:00:00:01", "10.0.4.1");
    auto* rA1 = routerA.addPort("eth1", "42:00:00:00:00:02", "10.0.8.1");
    routerA.addRoute({"10.0.4.0/24", "direct", "eth0", 0, false});
    routerA.addRoute({"10.0.8.0/24", "direct", "eth1", 0, false});

    Router routerB("R2");
    auto* rB0 = routerB.addPort("eth0", "42:00:00:00:00:03", "10.0.8.2");
    auto* rB1 = routerB.addPort("eth1", "42:00:00:00:00:04", "10.0.5.1");
    routerB.addRoute({"10.0.8.0/24", "direct", "eth0", 0, false});
    routerB.addRoute({"10.0.5.0/24", "direct", "eth1", 0, false});

    Switch rightSwitch("RightSwitch");
    auto* rs1 = rightSwitch.addPort("router", "43:00:00:00:00:01");
    auto* rs2 = rightSwitch.addPort("server", "43:00:00:00:00:02");

    Host client = makeHost("Client", "00:00:00:00:06:01", "10.0.4.10");
    Host server = makeHost("Server", "00:00:00:00:07:01", "10.0.5.10");
    client.installService(makeEchoService());
    server.installService(makeEchoService());
    server.installService(makeFileService());

    client.setDefaultGateway("10.0.4.1");
    server.setDefaultGateway("10.0.5.1");

    Link l1("Client-LeftSwitch");
    Link l2("LeftSwitch-Bridge");
    Link l3("Bridge-R1");
    Link l4("R1-R2");
    Link l5("R2-RightSwitch");
    Link l6("RightSwitch-Server");
    connect(l1, client.portByName("eth0"), ls1);
    connect(l2, ls2, br1);
    connect(l3, br2, rA0);
    connect(l4, rA1, rB0);
    connect(l5, rB1, rs1);
    connect(l6, rs2, server.portByName("eth0"));

    routerA.sendRipUpdate();
    routerB.sendRipUpdate();
    routerA.printRoutingTable();
    routerB.printRoutingTable();
    client.sendApplication("echo", "10.0.5.10", 7, "full stack hello");
    client.sendApplication("file", "10.0.5.10", 21, "routing-table.txt");
    bridge.printMacTable();
    leftSwitch.printMacTable();
    rightSwitch.printMacTable();
    printDomains(DomainAnalyzer::routedLan());
}

void Scenarios::runAll() {
    pointToPoint();
    hubStar();
    switchStar();
    hybridHubSwitch();
    fullStackRouted();
}

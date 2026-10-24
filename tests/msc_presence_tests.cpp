#include "msc_presence_policy.h"
#include <stdexcept>
#include <iostream>
using namespace msc_presence;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
    check(enabled_setting(true,true,false,false),"temporary third-person suppression cleared enabled tag");
    check(!enabled_setting(true,false,true,false),"requested view overrode explicit disabled preference");
    check(enabled_setting(false,false,true,false),"initial native enabled state ignored");
    check(enabled_setting(true,false,false,true),"first-person spectating did not enable tag");
    check(metadata_matches("reskate,v1,k3,jfp1,n-MSC-Midnight Skate Club-",3),"capability metadata rejected");
    check(!metadata_matches("reskate,k4,jfp1,nMSC",3),"wrong session secret accepted");
    check(!metadata_matches("reskate,k3,nname,jfp1",3),"capability in name accepted");
    check(!metadata_matches("reskate,k3,nMSC",3),"missing capability accepted");
    check(!metadata_matches("k3,jfp1,nMSC",3),"non-ReSkate metadata accepted");
    check(server_name("-MSC-Midnight Skate Club-24/7-Smoke-Sesh-VC-Build-Trainer"),"full name rejected");
    check(server_name(prefix) && server_name("-MSC-Midnight Skate Club-New suffix"),"suffix rejected");
    for(const auto name:{"Official","x-MSC-Midnight Skate Club-","-msc-Midnight Skate Club-","-MSC-Midnight Skate Club"})
        check(!server_name(name),"wrong name accepted");
    const Session a{1,2,3,4},b{5,6,7,8};
    Gate gate;gate.observe(a,true,prefix);check(!gate.due(true,0),"prefix without capability sent");
    gate.capability(b,true);check(!gate.due(true,0),"different server capability accepted" );
    gate.capability(a,true);check(gate.due(true,0)==true,"valid capability rejected" );
    check(dispatch_allowed(gate,a,a,"/jish-fp 1 1","/jish-fp 1 1"),"same-session dispatch refused");
    check(!dispatch_allowed(gate,b,a,"/jish-fp 1 1","/jish-fp 1 1"),"queued command leaked after server switch");
    check(!dispatch_allowed(gate,a,a,"/jish-fp 1 2","/jish-fp 1 1"),"unregistered report dispatched");
    gate.sent(true,100);check(!gate.due(true,5099),"heartbeat too early");
    check(gate.due(true,5100)==true,"heartbeat missing");
    check(gate.due(false,101)==false,"disable not immediate");gate.sent(false,101);
    check(!gate.due(false,100000),"disabled heartbeat sent");
    gate.observe(b,true,prefix);check(!gate.due(true,0),"confirmation crossed sessions");
    gate.capability(a,true);check(!gate.due(true,0),"old session capability reused");
    gate.capability(b,true);gate.observe(b,true,"Other server");check(!gate.due(true,0),"name mismatch retained state");
    gate.observe(b,true,prefix);check(!gate.due(true,0),"return to prefix reused capability");
    gate.capability(b,true);check(gate.due(true,0)==true,"fresh capability not accepted");
    gate.reset();check(!gate.due(true,0),"disconnect retained state");
    gate.observe(a,false,"");gate.capability(a,true);check(!gate.due(true,0),"unready session sent");
    gate.observe(a,true,prefix);check(gate.due(true,0)==true,"capability-before-name lost");
    gate.observe(a,false,prefix);check(!gate.due(true,0),"lost auth sent");    std::cout<<"MSC prefix, metadata, session, dispatch, state and heartbeat guards passed\n";
}

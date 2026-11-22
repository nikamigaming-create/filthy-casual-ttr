#include "eyebot_replay.hpp"
#include <cassert>
#include <cstdio>
using namespace eyebot;
int main(){
 Replay r; Lens l; assert(r.place(l)&&r.park()&&r.record(100,7,8192));
 Snapshot snaps; SnapTicket job{};
 assert(snaps.request(l,1)&&!snaps.request(l,1));
 assert(!snaps.take(job,false)&&snaps.take(job,true)&&!snaps.take(job,true));
 assert(!snaps.finish(job,2,true)&&!snaps.shown());
 assert(snaps.request(l,2)&&snaps.take(job,true)&&snaps.finish(job,2,true));
 assert(snaps.shown()==job.id&&!snaps.take(job,true));
 assert(snaps.request(l,2));snaps.cancel();assert(!snaps.take(job,true));
 Entity item[3]{};
 item[0].id=10;item[0].asset=100;item[0].centre={0,1,0};
 item[1].id=11;item[1].asset=101;item[1].centre={0,-1,0};
 item[2].id=12;item[2].asset=102;item[2].centre={2,1,0};item[2].radius=1;
 Pose hand;item[0].joints=&hand;item[0].jointCount=1;
 assert(r.sample(100,7,item,3)==Sample::Stored);
 assert(r.frames()[0].count==2&&r.actors()[0].id==10&&r.actors()[1].id==12);
 assert(r.sample(200,7,item,3)==Sample::Skipped);
 item[0].pose.position.x=.4f;hand.position.x=.2f;
 assert(r.sample(50000,7,item,3)==Sample::Stored);
 assert(r.joints()[0].position.x==0&&r.joints()[1].position.x==.2f);
 assert(r.lens().eye.x==0&&!r.place(l));
 assert(r.recall()&&r.save()&&!r.discard());
 unsigned submitted=0;assert(r.playback(24950,[&](uint64_t id,uint64_t,Pose pose,const Pose* joints,uint32_t n,uint32_t){
  ++submitted;
  if(id==10){assert(std::abs(pose.position.x-.2f)<.0001f);assert(n==1&&std::abs(joints[0].position.x-.1f)<.0001f);}
 }));
 assert(submitted==2&&r.finishExport(true)&&r.discard());
 assert(r.record(100000,7,8192));assert(r.sample(100000,7,item,3)==Sample::Stored);
 assert(r.sample(150000,8,item,3)==Sample::Incompatible&&r.state()==State::Review);
 assert(r.discard()&&r.record(500000,7,4096));
 uint64_t t=500000;
 while(r.sample(t,7,item,3)==Sample::Stored)t+=Replay::PeriodUs;
 assert(r.state()==State::Review&&r.frames().size()>1);
 std::puts("PASS: parked frustum, included/occluded-unknown bounds, pose history, 24 Hz cadence, budget, save and discard");
}

#include "../wrist_controls.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace wrist_controls;
static int checks=0;
static void require(bool ok,const char* text){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
static bool closeEnough(float a,float b){return std::abs(a-b)<.0001f;}
static bool closeEnough(Vec a,Vec b){return scene::distance(a,b)<.0001f;}
static Frame fixture(){
    Frame f;f.valid=mountingFrame({0,20,0},{0,0,0},{1,-4,0},{-1,-4,0},f.world);
    f.eye={0,0,35};f.facing=facingEye(f.world,f.eye);return f;
}
static void tip(Frame& f,int button,float depthCm=0){auto p=buttonLocal(button);p.z-=depthCm*Cm;f.fingertip=worldPoint(f.world,p);}
static void away(Frame& f){f.fingertip=worldPoint(f.world,{0,0,5*Cm});}
int main(){
    auto f=fixture();require(f.valid,"real nondegenerate cuff mounts");require(f.facing,"dorsal surface faces eye");
    require(closeEnough(axis(f.world,1),{0,1,0}),"Y runs from wrist toward elbow");
    require(closeEnough(axis(f.world,2),{0,0,1}),"normal is opposite right palm");
    require(closeEnough(f.world.p,{0,.04f*AssetUnitsPerMetre,.026f*AssetUnitsPerMetre}),"mount is anatomical offset");
    require(closeEnough(dot(axis(f.world,0),scene::cross(axis(f.world,1),axis(f.world,2))),1.f),"mount determinant positive one");
    require(closeEnough(dot(axis(f.world,0),axis(f.world,1)),0.f),"orthogonal frame");
    Vec shift{133,-47,72};Transform shifted{};
    require(mountingFrame(Vec{0,20,0}+shift,shift,Vec{1,-4,0}+shift,Vec{-1,-4,0}+shift,shifted),"translated rig mounts");
    require(closeEnough(shifted.p,f.world.p+shift),"mount follows native translation exactly");
    auto rotate=[](Vec p){return Vec{-p.y,p.x,p.z};};Transform turned{};
    require(mountingFrame(rotate({0,20,0}),rotate({0,0,0}),rotate({1,-4,0}),rotate({-1,-4,0}),turned),"rotated rig mounts");
    require(closeEnough(turned.p,rotate(f.world.p))&&closeEnough(axis(turned,0),rotate(axis(f.world,0))),"mount rotates with anatomy without eye input");
    Transform invalid{};require(!mountingFrame({0,0,0},{0,0,0},{1,1,1},{2,2,2},invalid),"degenerate rig rejected");
    require(!mountingFrame({0,20,0},{0,0,0},{1,1,1},{2,2,2},invalid),"collinear fingers rejected");
    require(!mountingFrame({0,20,0},{0,0,0},{1,-4,0},{-1,-4,0},invalid,std::numeric_limits<float>::quiet_NaN()),"invalid scene scale rejected");
    require(!facingEye(f.world,{0,0,-35}),"back face rejects intent");
    require(!facingEye(f.world,{0,0,350}),"distant wrist rejects intent");
    require(closeEnough(localPoint(f.world,buttonWorld(f,Settings)),buttonLocal(Settings)),"button mapping round trips");
    require(closeEnough(PressTravel,.16f*Cm),"asset local travel is exactly 1.6mm");
    tip(f,Weapons,-1.f);require(visualPress(f,Weapons)==0,"one centimetre air hover has no visual press");
    tip(f,Weapons,0);require(visualPress(f,Weapons)<.0001f,"surface contact without depression stays raised");
    tip(f,Weapons,.08f);require(closeEnough(visualPress(f,Weapons),.5f),"0.8mm depresses plunger halfway");
    tip(f,Weapons,.16f);require(closeEnough(visualPress(f,Weapons),1.f),"1.6mm reaches full travel");
    tip(f,Weapons,.22f);require(closeEnough(visualPress(f,Weapons),1.f),"tracking overtravel cannot extend mechanical travel");
    tip(f,Weapons,.90f);require(visualPress(f,Weapons)==0,"backside penetration cannot depress a button");
    auto bad=f;bad.fingertip.x=std::numeric_limits<float>::quiet_NaN();
    require(visualPress(bad,Weapons)==0,"nonfinite fingertip has no depression");
    tip(f,Weapons,.13f);bad=f;bad.eye={0,0,-35};bad.facing=false;
    require(visualPress(bad,Weapons)==0,"backfacing cap has no depression");
    tip(f,Weapons,.60f);require(closeEnough(visualPress(f,Weapons),1.f),"6mm tracking depth still clamps visual travel to 1.6mm");
    TouchState touch;tip(f,Weapons,.13f);
    require(touch.update(f,100)==None&&touch.update(f,400)==None,"initial depressed overlap cannot activate");
    away(f);require(touch.update(f,500)==None&&touch.armed,"front withdrawal arms");
    tip(f,Weapons,-1.f);touch.update(f,600);
    require(touch.update(f,900)==None,"hover duration never activates");
    tip(f,Weapons,0);touch.update(f,1000);require(touch.update(f,1100)==None,"mere contact never activates");
    tip(f,Weapons,.08f);require(touch.update(f,1200)==None,"half travel does not actuate");
    tip(f,Weapons,.13f);require(touch.update(f,1217)==Weapons,"physical threshold crossing activates without artificial dwell");
    require(visualPress(f,Weapons)>.8f,"plunger stays depressed after action");
    require(touch.update(f,1300)==None&&touch.update(f,1650)==None,"continued depression never repeats after cooldown");
    away(f);touch.update(f,1750);tip(f,Reload,.13f);touch.update(f,1850);
    require(touch.update(f,1950)==None,"lateral entry without observed front approach cannot activate");
    tip(f,Reload,-.7f);touch.update(f,2050);
    bad=f;bad.eye={0,0,-35};bad.facing=false;
    require(touch.update(bad,2100)==None&&!touch.armed,"look away cancels approach");
    tip(f,Reload,.13f);require(touch.update(f,2150)==None&&!touch.armed,"looking back at overlapping finger cannot activate");
    away(f);touch.update(f,2250);tip(f,Reload,-.7f);touch.update(f,2350);
    tip(f,Reload,.13f);require(touch.update(f,2950)==None&&!touch.armed,"stale sampling cannot create a crossing");
    away(f);touch.update(f,3050);tip(f,Reload,-.7f);touch.update(f,3150);
    tip(f,Reload,.9f);require(touch.update(f,3167)==None&&!touch.armed,"deeper than8mm tunnel disarms");
    tip(f,Reload,.13f);require(touch.update(f,3184)==None&&!touch.armed,"returning through backside cannot actuate");
    away(f);touch.update(f,3300);tip(f,Weapons,-.7f);touch.update(f,3400);
    require(touch.update(f,3450,false)==None&&!touch.armed,"native menu or tracking loss cancels approach");
    auto edge=buttonLocal(Weapons);edge.x+=buttonRadius(Weapons)+.01f*Cm;edge.z-=.13f*Cm;
    require(!physicalPress(edge,Weapons).valid,"outside physical cap radius cannot press");
    // A diagonal crossing outside the cap then a lateral move under its face is
    // not a physical top-face press, even if the broad approach zone was visited.
    TouchState lateral;away(f);lateral.update(f,100);
    edge=buttonLocal(Weapons);edge.x+=buttonRadius(Weapons)+.1f*Cm;edge.z+=.7f*Cm;
    f.fingertip=worldPoint(f.world,edge);lateral.update(f,200);
    edge.z=buttonLocal(Weapons).z-.2f*Cm;f.fingertip=worldPoint(f.world,edge);
    require(lateral.update(f,217)==None,"threshold crossing outside cap is rejected");
    tip(f,Weapons,.2f);require(lateral.update(f,234)==None,"lateral motion after outside crossing cannot activate");
    // Natural taps, including different sample phases: 0.1-0.3m/s at60/90Hz.
    // All are faster than the previous60ms band dwell, and must each fire once.
    int trajectories=0;
    for(int hz:{60,90})for(float speed:{.1f,.2f,.3f})for(int phase=0;phase<4;phase++)for(int action=0;action<4;action++){
        auto moving=fixture();TouchState press;away(moving);press.update(moving,100);
        const float step=speed*100.f/float(hz);
        float depth=-2.f+step*float(phase)*.25f;
        int events=0;std::uint64_t time=200;int frame=0;
        while(depth<.75f){
            tip(moving,action,depth);const int result=press.update(moving,time);
            if(result!=None){++events;require(result==action,"tap selects only intended cap");require(depth>=.12f&&depth<=.8f,"tap occurs within physical tracking volume");}
            if(depth<.12f)require(result==None,"no trajectory activates before threshold");
            depth+=step;++frame;time=200+std::uint64_t(std::llround(frame*1000.0/hz));
        }
        tip(moving,action,.75f);const int last=press.update(moving,time);
        if(last!=None){++events;require(last==action,"last fast sample selects intended cap");}
        for(int hold=1;hold<=50;hold++){
            const auto heldTime=time+std::uint64_t(std::llround(hold*1000.0/hz));
            if(press.update(moving,heldTime)!=None)++events;
        }
        require(events==1,"natural press produces exactly one activation including held cooldown");
        // Withdrawal plus cooldown re-enables a second independent press.
        time+=1000;away(moving);press.update(moving,time);
        // A long sampling gap deliberately resets; a second outside sample arms.
        press.update(moving,time+17);tip(moving,action,-.7f);press.update(moving,time+34);
        tip(moving,action,.5f);require(press.update(moving,time+51)==action,"fresh withdrawal re-enables another natural tap");
        ++trajectories;
    }
    std::printf("PASS %d wrist assertions; %d natural-velocity tap trajectories at60/90Hz\n",checks,trajectories);
}

#define main historical_fixture_main
#include "bridge_resolver_tests.cpp"
#undef main
#include "native_211.h"
int main(int argc,char** argv) {
    try {
        check(argc==3,"official 2.1.3 and previous 2.1.2 fixture paths required");
        auto image=load(argv[1]);const auto& profile=stock_contract::contracts[14];
        const auto digest=hash_of(profile.sha256);
        const auto good=resolve(image,digest);
        check(bool(good),good.error.c_str());check(good.known,"2.1.3 not recognized");
        check(good.read_callback==native211::read.rva && good.queue_callback==native211::queue.rva,"callback mappings changed");
        for(const auto& entry:native211::entries) {
            auto damaged=image;damaged.bytes[entry.rva]^=1;
            check(!resolve(damaged,digest,true),entry.role);
        }
        for(const auto at:{native211::status.rva+0x21,native211::status.rva+0x96,
                           native211::source_init.rva+0x60,native211::source_init.rva+0x81,native211::source_init.rva+0x88}) {
            auto damaged=image;damaged.bytes[at+3]^=1;
            check(!resolve(damaged,digest,true),"changed mutex/camera reference accepted");
        }
        auto wrong=digest;wrong[0]^=1;
        check(!resolve(image,wrong,true),"force detection accepted an unreviewed 2.1.3 image");
        auto bad=image;bad.status++;
        check(!resolve(bad,digest,true),"wrong status export accepted");
        auto previous=load(argv[2]);
        const auto previous_hash=hash_of("96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43");
        check(!stock_contract::find_contract(previous_hash,previous.status),"2.1.2 identity accepted by dedicated 2.1.3 profile");
        check(!resolve(previous,previous_hash,true),"forced resolution accepted previous 2.1.2 DLL");
        // Native partial reads preserve debug when the revision is unchanged.
        // Ensure our displayed master/offset values cannot feed back into it.
        std::vector<std::byte> model(26128,std::byte{0x5a});
        const auto before=model;
        native211::refresh_debug(model.data());
        for(std::size_t i=0;i<model.size();++i)
            check(model[i]==(i>=26104 && i<26112 ? std::byte{}:before[i]),"refresh damaged another model section/revision");
        std::uint64_t revision{};std::memcpy(&revision,model.data()+26104,8);
        bool shown_master=true,native_active=false;
        unsigned copies{};
        for(unsigned i=0;i<5;++i) {
            native211::refresh_debug(model.data());std::memcpy(&revision,model.data()+26104,8);
            if(revision!=7) {shown_master=native_active;++copies;}
            check(!shown_master,"partial update reused display-only checkbox as native activation");
            revision=7;std::memcpy(model.data()+26104,&revision,8);shown_master=true;
        }
        check(copies==5,"unchanged native revision skipped refresh");
        check(!native211::panel_controller_navigation,"companion controller navigation enabled");
        check(native211::capture_controller(false,true),"explicit recording did not capture stock navigation");
        check(!native211::capture_controller(true,true),"physical recording read was suppressed");
        check(!native211::capture_controller(false,false),"stock navigation did not resume");
        native211::RecordingCapture capture;
        check(capture.consume(false,true,true),"recording did not capture chord");
        check(!capture.consume(true,true,true),"raw read captured during recording");
        check(capture.consume(false,false,true),"cancelled chord leaked into stock menu before release");
        check(!capture.consume(false,false,false),"neutral release did not restore stock navigation");
        check(!capture.consume(false,false,true),"next stock press was suppressed");
        std::cout<<"2.1.3 identity, all native entry tamper guards, partial-model refresh and controller ownership passed.\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}

// File: SecondaryProducer.cc
// Description:  see SecondaryProducer.h
// Author:  Bill Tanenbaum
//
//--------------------------------------------

#include "IOPool/SecondaryInput/test/SecondaryProducer.h"
#include "DataFormats/Common/interface/ConvertHandle.h"
#include "DataFormats/TestObjects/interface/OtherThingCollection.h"
#include "DataFormats/TestObjects/interface/ThingCollection.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/InputSourceDescription.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Sources/interface/VectorInputSourceFactory.h"
#include "FWCore/Utilities/interface/TypeID.h"

#include <memory>

namespace edm {

  namespace {
    class Helper {
    public:
      Helper() : bh_(), en_() {}
      void operator()(EventPrincipal const&);
      BasicHandle bh_;
      EventNumber_t en_;
    };
    void
    Helper::operator()(EventPrincipal const& ep) {
      typedef edmtest::ThingCollection TC;
      en_ = ep.id().event();
      bh_ = ep.getByType(TypeID(typeid(TC)));
    }
  }

  // Constructor
  // make secondary input source
  SecondaryProducer::SecondaryProducer(ParameterSet const& pset) :
        secInput_(makeSecInput(pset)),
        sequential_(pset.getUntrackedParameter<bool>("sequential", false)),
        specified_(pset.getUntrackedParameter<bool>("specified", false)),
        firstEvent_(true),
        firstLoop_(true),
        expectedEventNumber_(1) {
    produces<edmtest::ThingCollection>();
    produces<edmtest::OtherThingCollection>("testUserTag");
  }

  // Virtual destructor needed.
  SecondaryProducer::~SecondaryProducer() {}

  // Functions that get called by framework every event
  void SecondaryProducer::produce(Event& e, EventSetup const&) {

    typedef edmtest::ThingCollection TC;
    typedef Wrapper<TC> WTC;

    Helper helper;

    if(sequential_) {
      secInput_->loopSequential(1, helper);
    } else if(specified_) {
      // Just for simplicity, we use the event ID from the primary to read the secondary.
      std::vector<EventID> events(1, e.id());
      secInput_->loopSpecified(events, helper);
    } else {
      secInput_->loopRandom(1, helper);
    }

    EventNumber_t en = helper.en_;
    BasicHandle bh = helper.bh_;
    assert(bh.isValid());
    if(!(bh.interface()->dynamicTypeInfo() == typeid(TC))) {
      handleimpl::throwConvertTypeError(typeid(TC), bh.interface()->dynamicTypeInfo());
    }
    WTC const* wtp = static_cast<WTC const*>(bh.wrapper());

    assert(wtp);
    TC const* tp = wtp->product();
    std::auto_ptr<TC> thing(new TC(*tp));

    // Put output into event
    e.put(thing);

    if(!sequential_ && !specified_ && firstLoop_ && en == 1) {
      expectedEventNumber_ = 1;
      firstLoop_ = false;
    }
    if(firstEvent_) {
      firstEvent_ = false;
      if(!sequential_ && !specified_) {
        expectedEventNumber_ = en;
      }
    }
    assert (expectedEventNumber_ == en);
    ++expectedEventNumber_;
  }

  boost::shared_ptr<VectorInputSource> SecondaryProducer::makeSecInput(ParameterSet const& ps) {
    ParameterSet const& sec_input = ps.getParameterSet("input");

    boost::shared_ptr<VectorInputSource> input_(static_cast<VectorInputSource *>
      (VectorInputSourceFactory::get()->makeVectorInputSource(sec_input,
      InputSourceDescription()).release()));
    return input_;
  }

} //edm
using edm::SecondaryProducer;
DEFINE_FWK_MODULE(SecondaryProducer);

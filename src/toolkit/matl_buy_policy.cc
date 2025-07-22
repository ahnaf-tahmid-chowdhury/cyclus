#include "matl_buy_policy.h"

#include <sstream>

#include "error.h"

#define LG(X) LOG(LEV_##X, "buypol")
#define LGH(X)                                                    \
  LOG(LEV_##X, "buypol") << "policy " << name_ << " (agent "      \
                         << Trader::manager()->prototype() << "-" \
                         << Trader::manager()->id() << "): "

namespace cyclus {
namespace toolkit {

MatlBuyPolicy::MatlBuyPolicy()
    : Trader(NULL),
      name_(""),
      throughput_(std::numeric_limits<double>::max()),
      quantize_(-1),
      fill_to_(std::numeric_limits<double>::max()),
      req_at_(std::numeric_limits<double>::max()),
      cumulative_cap_(-1),
      cycle_total_inv_(0),
      active_dist_(NULL),
      dormant_dist_(NULL),
      size_dist_(NULL) {
  Warn<EXPERIMENTAL_WARNING>(
      "MatlBuyPolicy is experimental and its API may be subject to change");
}

MatlBuyPolicy::~MatlBuyPolicy() {
  if (manager() != NULL) manager()->context()->UnregisterTrader(this);
}

MatlBuyPolicy& MatlBuyPolicy::ResetBehavior() {
  throughput_ = std::numeric_limits<double>::max();
  quantize_ = -1;
  fill_to_ = std::numeric_limits<double>::max();
  req_at_ = std::numeric_limits<double>::max();
  cumulative_cap_ = -1;
  cycle_total_inv_ = 0;
  active_dist_ = NULL;
  dormant_dist_ = NULL;
  size_dist_ = NULL;
  return *this;
}

void MatlBuyPolicy::set_manager(Agent* m) {
  if (m != NULL) {
    Trader::manager_ = m;
  } else {
    std::stringstream ss;
    ss << "No manager set on Buy Policy " << name_;
    throw ValueError(ss.str());
  }
}

void MatlBuyPolicy::set_total_inv_tracker(TotalInvTracker* t) {
  if (t == NULL) {
    std::vector<ResBuf<Material>*> bufs = {buf_};
    buf_tracker_->Init(bufs, buf_->capacity());
  } else if (!t->buf_in_tracker(buf_)) {
    std::stringstream ss;
    ss << "TotalInvTracker does not contain ResBuf used in buy policy";
    throw ValueError(ss.str());
  } else {
    buf_tracker_ = t;
  }
}

void MatlBuyPolicy::set_inv_policy(std::string inv_policy, double fill,
                                   double req_at) {
  set_req_at(req_at);
  std::transform(inv_policy.begin(), inv_policy.end(), inv_policy.begin(),
                 ::tolower);
  if ((inv_policy == "ss")) {
    set_fill_to(fill);
  } else if ((inv_policy == "rq") || (inv_policy == "qr")) {
    set_quantize(fill);
    // maximum amount that an RQ policy could achieve is req_at + fill
    set_fill_to(req_at + fill);
  } else {
    throw ValueError("Invalid inventory policy");
  }
}

void MatlBuyPolicy::set_fill_to(double x) {
  assert(x > 0);
  fill_to_ = std::min(x, buf_->capacity());
}

void MatlBuyPolicy::set_req_at(double x) {
  assert(x >= 0);
  req_at_ = std::min(x, buf_->capacity());
}

void MatlBuyPolicy::set_cumulative_cap(double x) {
  assert(x > 0);
  cumulative_cap_ = std::min(x, buf_->capacity());
}

void MatlBuyPolicy::set_quantize(double x) {
  assert(x != 0);
  quantize_ = x;
}

void MatlBuyPolicy::set_throughput(double x) {
  assert(x >= 0);
  throughput_ = x;
}

void MatlBuyPolicy::init_active_dormant() {
  if (active_dist_ == NULL) {
    active_dist_ = boost::shared_ptr<FixedIntDist>(new FixedIntDist(1));
  }
  if (dormant_dist_ == NULL) {
    dormant_dist_ = boost::shared_ptr<FixedIntDist>(new FixedIntDist(-1));
  }
  if (size_dist_ == NULL) {
    size_dist_ = boost::shared_ptr<FixedDoubleDist>(new FixedDoubleDist(1.0));
  }

  if (size_dist_->max() > 1) {
    throw ValueError("Size distribution cannot have a max greater than 1.");
  }

  SetNextActiveTime();
  LGH(INFO4) << "first active time end: " << next_active_end_ << std::endl;

  if (dormant_dist_->sample() < 0) {
    next_dormant_end_ = -1;
    LGH(INFO4) << "dormant length -1, always active" << std::endl;
  } else if (use_cumulative_capacity()) {
    next_dormant_end_ = -1;
    LGH(INFO4) << "dormant length set at -1 for first active period of "
                  "cumulative capacity cycle"
               << std::endl;
  } else {
    SetNextDormantTime();
    LGH(INFO4) << "first dormant time end: " << next_dormant_end_ << std::endl;
  }
}

MatlBuyPolicy& MatlBuyPolicy::Init(Agent* manager, ResBuf<Material>* buf,
                                   std::string name,
                                   TotalInvTracker* buf_tracker) {
  set_manager(manager);
  buf_ = buf;
  name_ = name;
  set_total_inv_tracker(buf_tracker);
  init_active_dormant();
  return *this;
}

MatlBuyPolicy& MatlBuyPolicy::Init(
    Agent* manager, ResBuf<Material>* buf, std::string name,
    TotalInvTracker* buf_tracker, double throughput,
    boost::shared_ptr<IntDistribution> active_dist,
    boost::shared_ptr<IntDistribution> dormant_dist,
    boost::shared_ptr<DoubleDistribution> size_dist) {
  set_manager(manager);
  buf_ = buf;
  name_ = name;
  set_total_inv_tracker(buf_tracker);
  set_throughput(throughput);
  active_dist_ = active_dist;
  dormant_dist_ = dormant_dist;
  size_dist_ = size_dist;
  init_active_dormant();
  return *this;
}

MatlBuyPolicy& MatlBuyPolicy::Init(Agent* manager, ResBuf<Material>* buf,
                                   std::string name,
                                   TotalInvTracker* buf_tracker,
                                   double throughput, double quantize) {
  set_manager(manager);
  buf_ = buf;
  name_ = name;
  set_total_inv_tracker(buf_tracker);
  set_throughput(throughput);
  set_quantize(quantize);
  init_active_dormant();
  return *this;
}

MatlBuyPolicy& MatlBuyPolicy::Init(Agent* manager, ResBuf<Material>* buf,
                                   std::string name,
                                   TotalInvTracker* buf_tracker,
                                   double throughput, std::string inv_policy,
                                   double fill_behav, double req_at) {
  set_manager(manager);
  buf_ = buf;
  name_ = name;
  set_total_inv_tracker(buf_tracker);
  set_inv_policy(inv_policy, fill_behav, req_at);
  set_throughput(throughput);
  init_active_dormant();
  return *this;
}

MatlBuyPolicy& MatlBuyPolicy::Init(Agent* manager, ResBuf<Material>* buf,
                                   std::string name,
                                   TotalInvTracker* buf_tracker,
                                   std::string inv_policy, double fill_behav,
                                   double req_at) {
  set_manager(manager);
  buf_ = buf;
  name_ = name;
  set_total_inv_tracker(buf_tracker);
  set_inv_policy(inv_policy, fill_behav, req_at);
  init_active_dormant();
  return *this;
}

MatlBuyPolicy& MatlBuyPolicy::Init(
    Agent* manager, ResBuf<Material>* buf, std::string name,
    TotalInvTracker* buf_tracker, double throughput, double cumulative_cap,
    boost::shared_ptr<IntDistribution> dormant_dist) {
  set_manager(manager);
  buf_ = buf;
  name_ = name;
  set_total_inv_tracker(buf_tracker);
  set_throughput(throughput);
  set_cumulative_cap(cumulative_cap);
  dormant_dist_ = dormant_dist;
  init_active_dormant();
  return *this;
}

MatlBuyPolicy& MatlBuyPolicy::Set(std::string commod) {
  CompMap c;
  c[10010000] = 1e-100;
  return Set(commod, Composition::CreateFromMass(c), 1.0);
}

MatlBuyPolicy& MatlBuyPolicy::Set(std::string commod, Composition::Ptr c) {
  return Set(commod, c, 1.0);
}

MatlBuyPolicy& MatlBuyPolicy::Set(std::string commod, Composition::Ptr c,
                                  double pref) {
  CommodDetail d;
  d.comp = c;
  d.pref = pref;
  commod_details_[commod] = d;
  return *this;
}

MatlBuyPolicy& MatlBuyPolicy::Unset(std::string commod) {
  commod_details_.erase(commod);
  return *this;
}

void MatlBuyPolicy::Start() {
  if (manager() == NULL) {
    std::stringstream ss;
    ss << "No manager set on Buy Policy " << name_;
    throw ValueError(ss.str());
  }
  manager()->context()->RegisterTrader(this);
}

void MatlBuyPolicy::Stop() {
  if (manager() == NULL) {
    std::stringstream ss;
    ss << "No manager set on Buy Policy " << name_;
    throw ValueError(ss.str());
  }
  manager()->context()->UnregisterTrader(this);
}

std::set<RequestPortfolio<Material>::Ptr> MatlBuyPolicy::GetMatlRequests() {
  rsrc_commods_.clear();
  std::set<RequestPortfolio<Material>::Ptr> ports;

  double amt = 0;

  int current_time_ = manager()->context()->time();

  double max_request_amt = use_cumulative_capacity()
                               ? cumulative_cap_ - cycle_total_inv_
                               : std::numeric_limits<double>::max();

  if (MakeReq() && !dormant(current_time_)) {
    amt = std::min(TotalAvailable() * SampleRequestSize(), max_request_amt);
  } else {
    LGH(INFO3) << "in dormant period, no request" << std::endl;
  }

  CheckActiveDormantCumulativeTimes();

  if (amt < eps()) return ports;

  bool excl = Excl();
  double req_amt = ReqQty(amt);
  int n_req = NReq(amt);
  LGH(INFO3) << "requesting " << amt << " kg via " << n_req << " request(s)"
             << std::endl;

  // one portfolio for each request
  for (int i = 0; i != n_req; i++) {
    RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
    std::vector<Request<Material>*> mreqs;
    std::map<std::string, CommodDetail>::iterator it;
    for (it = commod_details_.begin(); it != commod_details_.end(); ++it) {
      std::string commod = it->first;
      CommodDetail d = it->second;
      LG(INFO3) << "  - one " << amt << " kg request of " << commod;
      Material::Ptr m = Material::CreateUntracked(req_amt, d.comp);
      Request<Material>* r = port->AddRequest(m, this, commod, d.pref, excl);
      mreqs.push_back(r);
    }
    port->AddMutualReqs(mreqs);
    ports.insert(port);
  }

  return ports;
}

void MatlBuyPolicy::AcceptMatlTrades(
    const std::vector<std::pair<Trade<Material>, Material::Ptr>>& resps) {
  std::vector<std::pair<Trade<Material>, Material::Ptr>>::const_iterator it;
  rsrc_commods_.clear();
  for (it = resps.begin(); it != resps.end(); ++it) {
    rsrc_commods_[it->second] = it->first.request->commodity();
    LGH(INFO3) << "got " << it->second->quantity() << " kg of "
               << it->first.request->commodity() << std::endl;
    buf_->Push(it->second);
    // cumulative capacity handling
    if (use_cumulative_capacity()) {
      cycle_total_inv_ += it->second->quantity();
    }
  }
  // check if cumulative cap has been reached. If yes, then sample for dormant
  // length and reset cycle_total_inv
  if (use_cumulative_capacity() &&
      ((cumulative_cap_ - cycle_total_inv_) < eps_rsrc())) {
    SetNextDormantTime();
    LGH(INFO3) << "cycle cumulative inventory has been reached. Dormant period "
                  "will end at "
               << next_dormant_end_ << std::endl;
    cycle_total_inv_ = 0;
  }
}

void MatlBuyPolicy::SetNextActiveTime() {
  int active_length = active_dist_->sample();
  next_active_end_ = active_length + manager()->context()->time();
  if (manager() != NULL) {
    if (use_cumulative_capacity()) {
      RecordActiveDormantTime(manager()->context()->time(), "CumulativeCap",
                              active_length);
    } else {
      RecordActiveDormantTime(manager()->context()->time(), "Active",
                              active_length);
    }
  }
  return;
};

void MatlBuyPolicy::SetNextDormantTime() {
  int dormant_length;
  int dormant_start;
  if (use_cumulative_capacity()) {
    // cumulative_cap dormant portion is updated only after the active cycle
    // ends, because it's based on actual material recieved and not a dist
    // that can be sampled at any time. Therefore, need the +1 when
    // because next_active_end_ is not useful
    dormant_length = dormant_dist_->sample();
    dormant_start = manager()->context()->time() + 1;
  } else if (next_dormant_end_ >= 0) {
    // dormant dist is used, and so is active dist. Just need to sample for
    // length and add to active cycle
    dormant_length = dormant_dist_->sample();
    dormant_start = std::max(next_active_end_, 1);
  } else {  // next_active_end_ < 0 used to indicate always active. Do not enter
            // dormant
    return;
  }
  next_dormant_end_ = dormant_length + dormant_start;
  if (manager() != NULL) {
    RecordActiveDormantTime(dormant_start, "Dormant", dormant_length);
  }
  return;
}

double MatlBuyPolicy::SampleRequestSize() {
  return size_dist_->sample();
}

void MatlBuyPolicy::CheckActiveDormantCumulativeTimes() {
  if (manager()->context()->time() == next_dormant_end_) {
    if (use_cumulative_capacity()) {
      next_dormant_end_ = -1;
    } else {
      SetNextActiveTime();

      SetNextDormantTime();
      LGH(INFO4) << "end of dormant period, next active time end: "
                 << next_active_end_
                 << ", and next dormant time end: " << next_dormant_end_
                 << std::endl;
    }
  }
  return;
}

void MatlBuyPolicy::RecordActiveDormantTime(int time, std::string type,
                                            int length) {
  manager()
      ->context()
      ->NewDatum("BuyPolActiveDormant")
      ->AddVal("Agent", manager()->id())
      ->AddVal("Time", time)
      ->AddVal("Type", type)
      ->AddVal("Length", length)
      ->Record();
  return;
}

}  // namespace toolkit
}  // namespace cyclus

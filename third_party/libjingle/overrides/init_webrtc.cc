// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "init_webrtc.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/trace_event/trace_event.h"
#include "third_party/webrtc/overrides/webrtc/base/basictypes.h"
#include "third_party/webrtc/overrides/webrtc/base/logging.h"
#include "third_party/webrtc/system_wrappers/interface/event_tracer.h"

const unsigned char* GetCategoryGroupEnabled(const char* category_group) {
  return TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(category_group);
}

void AddTraceEvent(char phase,
                   const unsigned char* category_group_enabled,
                   const char* name,
                   unsigned long long id,
                   int num_args,
                   const char** arg_names,
                   const unsigned char* arg_types,
                   const unsigned long long* arg_values,
                   unsigned char flags) {
  TRACE_EVENT_API_ADD_TRACE_EVENT(phase, category_group_enabled, name, id,
                                  num_args, arg_names, arg_types, arg_values,
                                  NULL, flags);
}

namespace webrtc {
// Define webrtc::field_trial::FindFullName to provide webrtc with a field trial
// implementation.
namespace field_trial {
std::string FindFullName(const std::string& trial_name) {
  return base::FieldTrialList::FindFullName(trial_name);
}
}  // namespace field_trial

// Define webrtc::metrics functions to provide webrtc with implementations.
namespace metrics {

// This class doesn't actually exist, so don't go looking for it :)
// This type is just fwd declared here in order to use it as an opaque type
// between the Histogram functions in this file.
class Histogram;

Histogram* HistogramFactoryGetCounts(
    const std::string& name, int min, int max, int bucket_count) {
  return reinterpret_cast<Histogram*>(
      base::Histogram::FactoryGet(name, min, max, bucket_count,
          base::HistogramBase::kUmaTargetedHistogramFlag));
}

Histogram* HistogramFactoryGetEnumeration(
    const std::string& name, int boundary) {
  return reinterpret_cast<Histogram*>(
      base::LinearHistogram::FactoryGet(name, 1, boundary, boundary + 1,
          base::HistogramBase::kUmaTargetedHistogramFlag));
}

void HistogramAdd(
    Histogram* histogram_pointer, const std::string& name, int sample) {
  base::HistogramBase* ptr =
      reinterpret_cast<base::HistogramBase*>(histogram_pointer);
  // The name should not vary.
  DCHECK(ptr->histogram_name() == name);
  ptr->Add(sample);
}
}  // namespace metrics
}  // namespace webrtc

// libpeerconnection is being compiled as a static lib.  In this case
// we don't need to do any initializing but to keep things simple we
// provide an empty intialization routine so that this #ifdef doesn't
// have to be in other places.
bool InitializeWebRtcModule() {
  webrtc::SetupEventTracer(&GetCategoryGroupEnabled, &AddTraceEvent);
  return true;
}

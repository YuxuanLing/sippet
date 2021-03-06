// Copyright (c) 2013 The Sippet Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SIPPET_MESSAGE_HEADERS_DATE_H_
#define SIPPET_MESSAGE_HEADERS_DATE_H_

#include "base/time/time.h"
#include "sippet/message/header.h"
#include "sippet/message/headers/bits/single_value.h"
#include "sippet/base/format.h"
#include "sippet/base/raw_ostream.h"

namespace sippet {

class Date :
  public Header,
  public single_value<base::Time> {
 private:
  DISALLOW_ASSIGN(Date);
  Date(const Date &other);
  Date *DoClone() const override;

 public:
  Date();
  Date(const single_value::value_type &date);
  ~Date() override;

  scoped_ptr<Date> Clone() const {
    return scoped_ptr<Date>(DoClone());
  }

  void print(raw_ostream &os) const override;
};

} // End of sippet namespace

#endif // SIPPET_MESSAGE_HEADERS_DATE_H_

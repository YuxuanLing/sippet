// Copyright (c) 2013 The Sippet Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SIPPET_TRANSPORT_SERVER_TRANSACTION_H_
#define SIPPET_TRANSPORT_SERVER_TRANSACTION_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/ref_counted.h"
#include "sippet/message/response.h"
#include "sippet/transport/channel.h"

namespace sippet {

class ServerTransaction :
  public base::RefCountedThreadSafe<ServerTransaction> {
 private:
  DISALLOW_COPY_AND_ASSIGN(ServerTransaction);
 public:
  ServerTransaction() {}

  virtual const std::string& id() const = 0;

  virtual scoped_refptr<Channel> channel() const = 0;

  virtual void Start(const scoped_refptr<Request> &incoming_request) = 0;

  virtual void Send(const scoped_refptr<Response> &response) = 0;

  virtual void HandleIncomingRequest(
                    const scoped_refptr<Request> &request) = 0;

  virtual void Close() = 0;

 protected:
  friend class base::RefCountedThreadSafe<ServerTransaction>;
  virtual ~ServerTransaction() {}
};

} /// End of sippet namespace

#endif // SIPPET_TRANSPORT_SERVER_TRANSACTION_H_

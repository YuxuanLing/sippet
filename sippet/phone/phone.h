// Copyright (c) 2015 The Sippet Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SIPPET_PHONE_PHONE_H_
#define SIPPET_PHONE_PHONE_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "url/gurl.h"

#include "sippet/phone/call.h"

namespace sippet {
namespace phone {

class IceServer {
 public:
  IceServer() {}
  ~IceServer() {}

  explicit IceServer(const std::string& uri) :
    uri_(uri) {
  }
  
  explicit IceServer(const std::string& uri,
                     const std::string& username,
                     const std::string& password) :
    uri_(uri),
    username_(username),
    password_(password) {
  }

  // URI example: stun:stun.l.google.com:19302
  const std::string &uri() const { return uri_; }
  void set_uri(const std::string &value) { uri_ = value; }

  // STUN/TURN username
  const std::string &username() const { return username_; }
  void set_username(const std::string &value) { username_ = value; }

  // STUN/TURN password
  const std::string &password() const { return password_; }
  void set_password(const std::string &value) { password_ = value; }

 private:
  std::string uri_; 
  std::string username_;
  std::string password_;
};

// Phone settings
class Settings {
 public:
  typedef std::vector<IceServer> IceServers;
  typedef std::vector<GURL> RouteSet;

  Settings();
  ~Settings();

  // Enable/disable streaming encryption
  void set_disable_encryption(bool value) { disable_encryption_ = value; }
  bool disable_encryption() const { return disable_encryption_; }

  // Enable/disable SCTP data channels
  void set_disable_sctp_data_channels(bool value) {
    disable_sctp_data_channels_ = value;
  }
  bool disable_sctp_data_channels() const {
    return disable_sctp_data_channels_;
  }

  // ICE servers list
  IceServers &ice_servers() {
    return ice_servers_;
  }
  const IceServers &ice_servers() const {
    return ice_servers_;
  }

  // Route-set
  RouteSet &route_set() {
    return route_set_;
  }
  const RouteSet &route_set() const {
    return route_set_;
  }

 private:
  IceServers ice_servers_;
  bool disable_encryption_;
  bool disable_sctp_data_channels_;
  RouteSet route_set_;
};

// This class stores account data used for logging into the server.
class Account {
 public:
  Account() {}
  Account(const std::string& username,
          const std::string& password,
          const std::string& host) :
    username_(username),
    password_(password),
    host_(host) {
  }

  void set_username(const std::string &username) { username_ = username; }
  const std::string &username() const { return username_; }

  void set_password(const std::string &password) { password_ = password; }
  const std::string &password() const { return password_; }

  // Host has the following form:
  //
  //     host = scheme ":" host_part [ transport ]
  //     scheme = "sip" / "sips"
  //     host_part = hostname / ip_address
  //     transport = ";transport=" ( "UDP" / "TCP" / "WS" )
  //
  void set_host(const std::string &host) { host_ = host; }
  const std::string &host() const { return host_; }

 private:
  std::string username_;
  std::string password_;
  std::string host_;
};

// Phone Observer
class PhoneObserver {
 public:
  // Called to inform completion of the last login attempt
  virtual void OnNetworkError(int error_code) = 0;

  // Called to inform completion of the last login attempt
  virtual void OnLoginCompleted(int status_code,
                                const std::string& status_text) = 0;

  // Called on incoming calls
  virtual void OnIncomingCall(const scoped_refptr<Call>& call) = 0;

  // Called on call error
  virtual void OnCallError(int status_code,
                           const std::string& status_text,
                           const scoped_refptr<Call>& call) = 0;

  // Called when callee phone starts ringing
  virtual void OnCallRinging(const scoped_refptr<Call>& call) = 0;

  // Called when callee picks up the phone
  virtual void OnCallEstablished(const scoped_refptr<Call>& call) = 0;

  // Called when callee hangs up
  virtual void OnCallHungUp(const scoped_refptr<Call>& call) = 0;

 protected:
  // Dtor protected as objects shouldn't be deleted via this interface.
  ~PhoneObserver() {}
};

// Base phone class
class Phone :
  public base::RefCountedThreadSafe<Phone> {
 public:
  // Phone state: the life cycle of the phone
  enum State {
    kStateOffline,
    kStateConnecting,
    kStateOnline,
  };

  // Initialize the |Phone| system.
  static void Initialize();

  // Create a |Phone| instance.
  static scoped_refptr<Phone> Create(PhoneObserver *phone_observer);

  virtual State state() const = 0;

  // Initializes a |Phone| instance.
  virtual bool Init(const Settings& settings) = 0;

  // Login the account.
  virtual bool Login(const Account &account) = 0;

  // Starts a call to the given destination.
  virtual scoped_refptr<Call> MakeCall(const std::string& destination) = 0;
    
  // Hangs up incoming and all active calls.
  virtual void HangUpAll() = 0;

  // Hangup all active calls and logout account.
  virtual void Logout() = 0;

 protected:
  friend class base::RefCountedThreadSafe<Phone>;
  virtual ~Phone() {}
};

} // namespace sippet
} // namespace phone

#endif // SIPPET_PHONE_PHONE_H_
